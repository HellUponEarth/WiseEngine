#include "Jobs/JobSystem.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <semaphore>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {
// Code Modification - Tests: Verify ownership, dependency order, backpressure, and drain-before-join behavior.
// Scope: Jobs integration tests. Performance: blocking gates and deadline checks are test-only.
// Toggle: WISEENGINE_BUILD_TESTS.
using namespace std::chrono_literals;
using CE::Jobs::Completion;
using CE::Jobs::CpuJob;
using CE::Jobs::Job;
using CE::Jobs::JobBytes;
using CE::Jobs::JobSystem;
using CE::Jobs::SubmitResult;

void require(bool passed, std::string_view message)
{
    if (!passed) {
        throw std::runtime_error(std::string(message));
    }
}

Completion receive(JobSystem& jobs)
{
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    do {
        if (auto completion = jobs.poll()) {
            return std::move(*completion);
        }
        std::this_thread::sleep_for(1ms);
    } while (std::chrono::steady_clock::now() < deadline);
    throw std::runtime_error("Timed out waiting for a completion");
}

void lanesAndDependencies()
{
    std::binary_semaphore ioStarted(0);
    std::binary_semaphore releaseIo(0);
    std::atomic<int> phases{};
    std::thread::id ioThread;
    const auto owner = std::this_thread::get_id();
    JobSystem jobs(1);

    require(jobs.submitIo({10, [&] {
        ioThread = std::this_thread::get_id();
        ioStarted.release();
        require(releaseIo.try_acquire_for(5s), "Test did not release I/O");
        return JobBytes{std::byte{10}};
    }}) == SubmitResult::Accepted, "I/O submission failed");
    require(ioStarted.try_acquire_for(3s), "SDL I/O worker did not start");
    require(ioThread != owner, "Blocking callback ran on the owner");

    std::vector<CpuJob> invalid{{{99, [] { return JobBytes{}; }}, {0}}};
    require(jobs.submitCpu(std::move(invalid)) == SubmitResult::Invalid && invalid.front().job.work,
        "A self-edge must be rejected without consuming the callback");

    std::vector<CpuJob> graph{
        {{1, [&] {
            require(std::this_thread::get_id() != owner && std::this_thread::get_id() != ioThread,
                "CPU work must progress on its own worker while I/O is blocked");
            phases.fetch_or(1);
            return JobBytes{std::byte{1}};
        }}, {}},
        {{2, [&] {
            phases.fetch_or(2);
            return JobBytes{std::byte{2}};
        }}, {}},
        {{3, [&] {
            require(phases.load() == 3, "A fan-in job ran before its prerequisites");
            return JobBytes{std::byte{3}};
        }}, {0, 1}},
        {{4, []() -> JobBytes { throw std::runtime_error("expected worker failure"); }}, {2}},
        {{5, [] { return JobBytes{std::byte{5}}; }}, {0}},
    };
    require(jobs.submitCpu(std::move(graph)) == SubmitResult::Accepted, "CPU graph submission failed");

    unsigned seen = 0;
    for (unsigned index = 0; index < 5; ++index) {
        const auto completion = receive(jobs);
        const auto tag = completion.tag;
        require(tag >= 1 && tag <= 5 && (seen & (1U << tag)) == 0, "Unexpected or duplicate CPU completion");
        if (tag == 4) {
            require(completion.error && (seen & (1U << 3)), "Expected ordered failure completion");
        } else {
            require(!completion.error && completion.bytes == JobBytes{static_cast<std::byte>(tag)},
                "CPU result payload or prerequisite ordering was incorrect");
        }
        seen |= 1U << tag;
    }

    releaseIo.release();
    const auto completion = receive(jobs);
    require(completion.tag == 10 && !completion.error && completion.bytes == JobBytes{std::byte{10}},
        "Blocked I/O did not return its payload to the owner");
}

void capacityAndShutdown()
{
    std::binary_semaphore releaseIo(0);
    std::atomic<std::size_t> ran{};
    JobSystem jobs(2);
    const auto work = [&] {
        ran.fetch_add(1);
        return JobBytes{};
    };
    require(jobs.submitCpu({{{1, work}, {}}, {{2, work}, {0}}}) == SubmitResult::Accepted,
        "CPU batch was rejected");
    require(jobs.submitCpu({{{3, work}, {}}}) == SubmitResult::Accepted, "Second CPU batch was rejected");
    require(jobs.submitIo({0, [&] {
        require(releaseIo.try_acquire_for(5s), "Shutdown test did not release I/O");
        return work();
    }}) == SubmitResult::Accepted, "Gated I/O slot was rejected");
    for (std::size_t index = 4; index < JobSystem::Capacity; ++index) {
        require(jobs.submitIo({0, work}) == SubmitResult::Accepted, "Available I/O slot was rejected");
    }

    Job retry{77, work};
    require(jobs.submitIo(std::move(retry)) == SubmitResult::Full && retry.work,
        "The shared capacity limit must reject without consuming the callback");
    const auto first = receive(jobs);
    require(!first.error && jobs.submitIo(std::move(retry)) == SubmitResult::Accepted,
        "Consuming a completion must return one admission credit");

    require(ran.load() <= 3, "I/O work was supposed to be outstanding before shutdown");
    const std::jthread unblockIo([&] {
        std::this_thread::sleep_for(25ms);
        releaseIo.release();
    });
    jobs.shutdown();
    jobs.shutdown();
    require(jobs.submitIo({0, work}) == SubmitResult::Closed &&
        jobs.submitCpu({{{0, work}, {}}}) == SubmitResult::Closed, "Shutdown did not close admission");
    require(ran.load() == JobSystem::Capacity + 1, "Shutdown did not finish accepted work");

    std::size_t drained = 0;
    bool foundRetry = false;
    while (const auto completion = jobs.poll()) {
        require(!completion->error, "Unexpected callback error during shutdown drain");
        foundRetry = foundRetry || completion->tag == 77;
        ++drained;
    }
    require(drained == JobSystem::Capacity && foundRetry, "Shutdown lost or duplicated completions");
}
// End of Code Modification - Tests
}

int main()
{
    try {
        lanesAndDependencies();
        capacityAndShutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
