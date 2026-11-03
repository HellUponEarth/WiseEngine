#include "JobSystem.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_thread.h>
#include <TaskScheduler.h>
#include <boost/container/static_vector.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace CE::Jobs {
namespace {
// Code Modification - Jobs: Reserve one completion slot per accepted job so workers never wait for the owner to drain.
// Scope: fixed-size queue storage. Performance: constant-time push/pop with no queue allocation. Toggle: none.
template<class Value>
class Queue {
public:
    bool empty() const noexcept { return size_ == 0; }

    void push(Value value) noexcept
    {
        assert(size_ < values_.size());
        values_[(head_ + size_) % values_.size()] = std::move(value);
        ++size_;
    }

    Value pop() noexcept
    {
        assert(!empty());
        Value value = std::move(values_[head_]);
        head_ = (head_ + 1) % values_.size();
        --size_;
        return value;
    }

private:
    std::array<Value, JobSystem::Capacity> values_{};
    std::size_t head_{};
    std::size_t size_{};
};
// End of Code Modification - Jobs
}

// Code Modification - Jobs: Own the enkiTS graphs and the SDL thread independently.
// Scope: scheduler internals. Performance: one parked I/O thread, configured CPU workers, one completion lock per job.
// Toggle: scheduler lifetime and CPU worker count.
struct JobSystem::State {
    struct CpuTask final : enki::ITaskSet {
        State* owner{};
        Job job;

        void ExecuteRange(enki::TaskSetPartition, std::uint32_t) override
        {
            owner->run(std::move(job));
        }
    };

    struct StartTask final : enki::ITaskSet {
        void ExecuteRange(enki::TaskSetPartition, std::uint32_t) override {}
    };

    struct Fence final : enki::ICompletable {
        void OnDependenciesComplete(enki::TaskScheduler* scheduler, std::uint32_t thread) override
        {
            // Acquire the enkiTS dependency release sequence before the owner can reclaim the graph.
            std::atomic_thread_fence(std::memory_order_acquire);
            enki::ICompletable::OnDependenciesComplete(scheduler, thread);
        }
    };

    struct Batch {
        Batch(std::size_t count, std::size_t edgeCount) : tasks(count), edges(edgeCount) {}

        StartTask start;
        Fence fence;
        std::vector<CpuTask> tasks;
        // Destroy edges first so enkiTS can unlink them while all nodes still exist.
        std::vector<enki::Dependency> edges;
    };

    explicit State(std::uint32_t cpuWorkers)
    {
        scheduler.Initialize(cpuWorkers + 1);
        ioThread = SDL_CreateThread(&ioEntry, "WiseEngine I/O", this);
        if (!ioThread) {
            throw std::runtime_error(std::string("SDL I/O thread creation failed: ") + SDL_GetError());
        }
    }

    static int SDLCALL ioEntry(void* context)
    {
        static_cast<State*>(context)->runIo();
        return 0;
    }

    void runIo()
    {
        for (;;) {
            Job job;
            {
                std::unique_lock lock(ioMutex);
                ioReady.wait(lock, [this] { return stopping || !ioQueue.empty(); });
                if (ioQueue.empty()) {
                    return;
                }
                job = ioQueue.pop();
            }
            run(std::move(job));
        }
    }

    void run(Job job)
    {
        Completion completion;
        completion.tag = job.tag;
        try {
            completion.bytes = job.work();
        } catch (...) {
            completion.error = std::current_exception();
        }
        std::lock_guard lock(completionMutex);
        completions.push(std::move(completion));
    }

    void reapBatches()
    {
        const auto finished = std::remove_if(batches.begin(), batches.end(),
            [](const auto& batch) { return batch->fence.GetIsComplete(); });
        batches.erase(finished, batches.end());
    }

    const std::thread::id owner = std::this_thread::get_id();
    enki::TaskScheduler scheduler;
    // Fixed storage bounds graph ownership without reallocating the owner list.
    boost::container::static_vector<std::unique_ptr<Batch>, Capacity> batches;
    std::size_t outstanding{};
    SDL_Thread* ioThread{};
    std::mutex ioMutex;
    std::condition_variable ioReady;
    Queue<Job> ioQueue;
    bool stopping{};
    std::mutex completionMutex;
    Queue<Completion> completions;
};
// End of Code Modification - Jobs

// Code Modification - Jobs: Validate graphs before publishing and drain accepted work on shutdown.
// Scope: owner-thread entry points. Performance: bounded graph construction and queue operations.
// Toggle: explicit lifetime, no work on the idle shell path.
JobSystem::JobSystem(std::uint32_t cpuWorkers)
{
    if (cpuWorkers == 0 || cpuWorkers > MaxCpuWorkers) {
        throw std::invalid_argument("JobSystem CPU worker count is out of range");
    }
    state_ = std::make_unique<State>(cpuWorkers);
}

JobSystem::~JobSystem()
{
    shutdown();
}

SubmitResult JobSystem::submitCpu(std::vector<CpuJob>&& batch)
{
    auto& state = *state_;
    assert(state.owner == std::this_thread::get_id());
    if (!state.ioThread) {
        return SubmitResult::Closed;
    }
    if (batch.empty() || batch.size() > Capacity) {
        return SubmitResult::Invalid;
    }
    state.reapBatches();
    if (batch.size() > Capacity - state.outstanding || state.batches.size() == Capacity) {
        return SubmitResult::Full;
    }
    std::size_t edgeCount = batch.size();
    for (std::size_t index = 0; index < batch.size(); ++index) {
        const auto& node = batch[index];
        if (!node.job.work) {
            return SubmitResult::Invalid;
        }
        std::size_t nextMinimum = 0;
        for (const auto prerequisite : node.prerequisites) {
            if (prerequisite < nextMinimum || prerequisite >= index) {
                return SubmitResult::Invalid;
            }
            nextMinimum = prerequisite + 1;
        }
        edgeCount += std::max(std::size_t{1}, node.prerequisites.size());
    }

    auto graph = std::make_unique<State::Batch>(batch.size(), edgeCount);
    std::size_t edge = 0;
    // enkiTS prepends outgoing edges. Install the fence first so it runs after every outgoing notification.
    for (auto& task : graph->tasks) {
        graph->fence.SetDependency(graph->edges[edge++], &task);
    }
    for (std::size_t index = 0; index < batch.size(); ++index) {
        auto& task = graph->tasks[index];
        auto& node = batch[index];
        task.owner = &state;
        task.job = std::move(node.job);
        if (node.prerequisites.empty()) {
            task.SetDependency(graph->edges[edge++], &graph->start);
        }
        for (const auto prerequisite : node.prerequisites) {
            task.SetDependency(graph->edges[edge++], &graph->tasks[prerequisite]);
        }
    }
    assert(edge == edgeCount);
    state.outstanding += batch.size();
    auto& accepted = *state.batches.emplace_back(std::move(graph));
    state.scheduler.AddTaskSetToPipe(&accepted.start);
    return SubmitResult::Accepted;
}

SubmitResult JobSystem::submitIo(Job&& job)
{
    auto& state = *state_;
    assert(state.owner == std::this_thread::get_id());
    if (!state.ioThread) {
        return SubmitResult::Closed;
    }
    if (!job.work) {
        return SubmitResult::Invalid;
    }
    if (state.outstanding == Capacity) {
        return SubmitResult::Full;
    }
    ++state.outstanding;
    {
        std::lock_guard lock(state.ioMutex);
        state.ioQueue.push(std::move(job));
    }
    state.ioReady.notify_one();
    return SubmitResult::Accepted;
}

std::optional<Completion> JobSystem::poll()
{
    auto& state = *state_;
    assert(state.owner == std::this_thread::get_id());
    state.reapBatches();
    std::lock_guard lock(state.completionMutex);
    if (state.completions.empty()) {
        return std::nullopt;
    }
    --state.outstanding;
    return state.completions.pop();
}

void JobSystem::shutdown() noexcept
{
    auto& state = *state_;
    assert(state.owner == std::this_thread::get_id());
    if (!state.ioThread) {
        return;
    }
    {
        std::lock_guard lock(state.ioMutex);
        state.stopping = true;
    }
    state.ioReady.notify_one();
    for (const auto& batch : state.batches) {
        state.scheduler.WaitforTask(&batch->fence);
    }
    state.scheduler.WaitforAllAndShutdown();
    SDL_WaitThread(state.ioThread, nullptr);
    state.ioThread = nullptr;
    state.batches.clear();
}
// End of Code Modification - Jobs
}
