#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace CE::Jobs {
// Code Modification - Jobs: Separate finite CPU work from blocking I/O and return results to their owner.
// Scope: owner-thread service API. Performance: worker threads and bounded queues when constructed.
// Toggle: construct only for workloads that need jobs. CPU worker count is configurable.
using JobBytes = std::vector<std::byte>;

struct Job {
    std::uint64_t tag{};
    std::function<JobBytes()> work;
};

struct CpuJob {
    Job job;
    std::vector<std::size_t> prerequisites;
};

struct Completion {
    std::uint64_t tag{};
    JobBytes bytes;
    std::exception_ptr error;
};

enum class SubmitResult { Accepted, Closed, Full, Invalid };

class JobSystem {
public:
    static constexpr std::size_t Capacity = 64;
    static constexpr std::uint32_t MaxCpuWorkers = 16;

    explicit JobSystem(std::uint32_t cpuWorkers);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // Use this API only from the constructing thread. Work must never call the service.
    // CPU edges are sorted, unique indices of earlier jobs in the same batch.
    // Accepted submissions move the callbacks. Rejected submissions leave the inputs available for retry.
    SubmitResult submitCpu(std::vector<CpuJob>&& batch);
    SubmitResult submitIo(Job&& job);
    std::optional<Completion> poll();
    void shutdown() noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};
// End of Code Modification - Jobs
}
