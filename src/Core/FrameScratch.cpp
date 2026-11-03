#include "FrameScratch.h"

#include <atomic>
#include <bit>
#include <new>
#include <utility>

namespace CE::Core {
// Code Modification - Memory: Retain aligned backing storage across an owner-to-consumer handoff.
// Scope: one arena. Performance: no heap allocation per bump or reset. Toggle: explicit arena construction.
struct FrameScratch::State {
    explicit State(std::size_t byteCapacity)
        : data(byteCapacity ? static_cast<std::byte*>(::operator new(byteCapacity, std::align_val_t{MaxAlignment})) : nullptr),
          capacity(byteCapacity)
    {
    }

    ~State()
    {
        if (data) {
            ::operator delete(data, std::align_val_t{MaxAlignment});
        }
    }

    std::byte* const data;
    const std::size_t capacity;
    std::atomic<bool> reading{};
};

FrameScratch::ReadLease::ReadLease(std::shared_ptr<State> state) noexcept : state_(std::move(state))
{
}

FrameScratch::ReadLease::ReadLease(ReadLease&& other) noexcept = default;

FrameScratch::ReadLease::~ReadLease()
{
    if (state_) {
        state_->reading.store(false, std::memory_order_release);
    }
}

FrameScratch::ReadLease::operator bool() const noexcept
{
    return static_cast<bool>(state_);
}

FrameScratch::FrameScratch(std::size_t capacity) : state_(std::make_shared<State>(capacity))
{
}

FrameScratch::~FrameScratch() = default;

std::span<std::byte> FrameScratch::allocate(std::size_t size, std::size_t alignment) noexcept
{
    if (sealed_ || size == 0 || size > capacity() - used_ || alignment > MaxAlignment || !std::has_single_bit(alignment)) {
        return {};
    }
    void* next = state_->data + used_;
    auto available = capacity() - used_;
    if (!std::align(alignment, size, next, available)) {
        return {};
    }
    used_ = capacity() - available + size;
    return {static_cast<std::byte*>(next), size};
}

FrameScratch::ReadLease FrameScratch::seal() noexcept
{
    if (sealed_) {
        return ReadLease(nullptr);
    }
    sealed_ = true;
    state_->reading.store(true, std::memory_order_relaxed);
    return ReadLease(state_);
}

bool FrameScratch::tryReset() noexcept
{
    // Pair with the lease's release so subsequent writes follow the consumer's last access.
    if (sealed_ && state_->reading.load(std::memory_order_acquire)) {
        return false;
    }
    used_ = 0;
    sealed_ = false;
    return true;
}

std::size_t FrameScratch::capacity() const noexcept
{
    return state_->capacity;
}

std::size_t FrameScratch::used() const noexcept
{
    return used_;
}
// End of Code Modification - Memory
}
