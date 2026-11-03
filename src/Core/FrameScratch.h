#pragma once

#include <cstddef>
#include <memory>
#include <span>

namespace CE::Core {
// Code Modification - Memory: Keep bounded transient frame storage alive until the consumer releases it.
// Scope: one owner and one transferable consumer lease per arena. Performance: one backing allocation and an atomic handoff.
// Toggle: construct only for transient workloads and choose a byte capacity.
class FrameScratch {
    struct State;

public:
    static constexpr std::size_t MaxAlignment = 64;

    class ReadLease {
    public:
        ~ReadLease();
        ReadLease(ReadLease&& other) noexcept;
        ReadLease& operator=(ReadLease&&) = delete;
        ReadLease(const ReadLease&) = delete;
        ReadLease& operator=(const ReadLease&) = delete;

        explicit operator bool() const noexcept;

    private:
        friend class FrameScratch;
        explicit ReadLease(std::shared_ptr<State> state) noexcept;
        std::shared_ptr<State> state_;
    };

    explicit FrameScratch(std::size_t capacity);
    ~FrameScratch();

    FrameScratch(const FrameScratch&) = delete;
    FrameScratch& operator=(const FrameScratch&) = delete;

    std::span<std::byte> allocate(std::size_t size, std::size_t alignment = 1) noexcept;
    [[nodiscard]] ReadLease seal() noexcept;
    bool tryReset() noexcept;
    std::size_t capacity() const noexcept;
    std::size_t used() const noexcept;

private:
    std::shared_ptr<State> state_;
    std::size_t used_{};
    bool sealed_{};
};
// End of Code Modification - Memory
}
