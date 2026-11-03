#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace CE::Core {
// Code Modification - Identity: Reject stale or foreign handles without relocating live objects.
// Scope: fixed-capacity owner-thread tables. Performance: one slot array and constant-time lookup, insertion and removal.
// Toggle: caller-selected capacity.
namespace Detail {
std::uint64_t allocateHandleDomain();
}

template<class T>
struct Handle {
    std::uint64_t domain{};
    std::uint32_t index{};
    std::uint32_t generation{};

    bool operator==(const Handle&) const = default;
};

template<class T>
class HandleTable {
    static_assert(std::is_nothrow_destructible_v<T>);

public:
    explicit HandleTable(std::uint32_t capacity)
        : slots_(capacity), domain_(Detail::allocateHandleDomain()), freeHead_(capacity ? 0 : NoSlot)
    {
        for (std::uint32_t index = 0; index < capacity; ++index) {
            slots_[index].next = index + 1 < capacity ? index + 1 : NoSlot;
        }
    }

    HandleTable(const HandleTable&) = delete;
    HandleTable& operator=(const HandleTable&) = delete;

    template<class... Args>
    std::optional<Handle<T>> emplace(Args&&... args)
    {
        if (freeHead_ == NoSlot) {
            return std::nullopt;
        }
        const auto index = freeHead_;
        auto& slot = slots_[index];
        // Commit the free-list change only after construction succeeds.
        slot.value.emplace(std::forward<Args>(args)...);
        freeHead_ = slot.next;
        ++size_;
        return Handle<T>{domain_, index, slot.generation};
    }

    const T* get(Handle<T> handle) const noexcept
    {
        if (handle.domain != domain_ || handle.index >= slots_.size()) {
            return nullptr;
        }
        const auto& slot = slots_[handle.index];
        return slot.generation == handle.generation && slot.value ? std::addressof(*slot.value) : nullptr;
    }

    T* get(Handle<T> handle) noexcept
    {
        return const_cast<T*>(std::as_const(*this).get(handle));
    }

    bool erase(Handle<T> handle) noexcept
    {
        if (!get(handle)) {
            return false;
        }
        auto& slot = slots_[handle.index];
        slot.value.reset();
        --size_;
        // Retire exhausted slots instead of reissuing a generation that old handles may still carry.
        if (slot.generation < std::numeric_limits<std::uint32_t>::max()) {
            ++slot.generation;
            slot.next = freeHead_;
            freeHead_ = handle.index;
        }
        return true;
    }

    std::size_t capacity() const noexcept { return slots_.size(); }
    std::size_t size() const noexcept { return size_; }

private:
    static constexpr auto NoSlot = std::numeric_limits<std::uint32_t>::max();
    struct Slot {
        std::optional<T> value;
        std::uint32_t generation{1};
        std::uint32_t next{NoSlot};
    };

    std::vector<Slot> slots_;
    const std::uint64_t domain_;
    std::uint32_t freeHead_;
    std::size_t size_{};
};
// End of Code Modification - Identity
}
