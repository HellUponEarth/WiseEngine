#include "Core/Core.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {
// Code Modification - Tests: Exercise capacity, stale identities, and scratch lifetime across threads.
// Scope: Core contract test. Performance: test-only worker and synchronization. Toggle: WISEENGINE_BUILD_TESTS.
using CE::Core::FrameScratch;
using CE::Core::Handle;
using CE::Core::HandleTable;

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Value {
    explicit Value(int input) : data(input)
    {
        if (input < 0) {
            throw std::invalid_argument("Negative test value");
        }
    }

    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    int data;
};

void handles()
{
    HandleTable<Value> empty(0);
    require(!empty.emplace(1), "Zero-capacity table accepted an object");
    HandleTable<Value> table(2);
    const auto first = table.emplace(7);
    require(first && table.get(*first), "Initial handle did not resolve");
    const auto* firstAddress = table.get(*first);

    bool rejected = false;
    try {
        (void)table.emplace(-1);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && table.size() == 1, "Throwing constructor consumed a slot");
    const auto second = table.emplace(8);
    require(second && table.size() == table.capacity() && !table.emplace(9), "Capacity was not enforced");
    require(table.get(*first) == firstAddress && firstAddress->data == 7, "Insertion moved a live object");

    require(!table.get({}) && !table.get({first->domain, std::numeric_limits<std::uint32_t>::max(), first->generation}),
        "Invalid handles must not resolve");
    require(table.erase(*first) && !table.erase(*first) && !table.get(*first), "Erased handle stayed valid");
    const auto replacement = table.emplace(9);
    require(replacement && replacement->index == first->index && replacement->generation != first->generation &&
        !table.get(*first) && table.get(*replacement) && table.get(*replacement)->data == 9,
        "Reused slot accepted the old generation");

    std::optional<Handle<Value>> foreign;
    {
        HandleTable<Value> temporary(1);
        foreign = temporary.emplace(10);
        require(foreign && !table.get(*foreign) && !temporary.get(*replacement), "Foreign handle resolved");
    }
    HandleTable<Value> newTable(1);
    require(newTable.emplace(11).has_value() && !newTable.get(*foreign), "A new table accepted a destroyed table's handle");
}

void scratch()
{
    FrameScratch empty(0);
    require(empty.allocate(1).empty(), "Zero-capacity scratch accepted an allocation");
    FrameScratch arena(80);
    auto first = arena.allocate(1);
    require(first.size() == 1, "Initial scratch allocation failed");
    require(arena.allocate(17, FrameScratch::MaxAlignment).empty() && arena.used() == 1,
        "Alignment padding must count toward the capacity without consuming space on failure");
    auto aligned = arena.allocate(16, FrameScratch::MaxAlignment);
    require(aligned.size() == 16 && reinterpret_cast<std::uintptr_t>(aligned.data()) % 64 == 0 &&
        arena.used() == arena.capacity(), "Alignment or padding accounting is incorrect");
    require(arena.allocate(1).empty() && arena.used() == 80, "Exhaustion changed the cursor");
    aligned.front() = std::byte{42};
    const std::span<const std::byte> published = aligned;

    std::binary_semaphore release(0);
    std::binary_semaphore released(0);
    bool received = false;
    auto lease = arena.seal();
    require(static_cast<bool>(lease) && !arena.seal() && !arena.tryReset() && arena.allocate(1).empty(),
        "Sealing did not prevent allocation and reset");
    std::jthread consumer([token = std::move(lease), &release, &released, &received, published]() mutable {
        {
            const auto owned = std::move(token);
            release.acquire();
            received = static_cast<bool>(owned) && published.front() == std::byte{42};
        }
        released.release();
    });

    const bool resetBlocked = !arena.tryReset();
    release.release();
    released.acquire();
    require(resetBlocked && received && arena.tryReset() && arena.used() == 0,
        "Scratch was not retained until the consumer released it");
    require(arena.allocate(0).empty() && arena.allocate(1, 0).empty() && arena.allocate(1, 3).empty() &&
        arena.allocate(1, 128).empty() && arena.used() == 0, "Invalid requests changed the cursor");
    require(arena.allocate(1).data() == first.data() && arena.tryReset(), "Reset did not reuse storage");
}

void leaseRetainsBacking()
{
    std::optional<FrameScratch::ReadLease> lease;
    std::span<const std::byte> published;
    {
        FrameScratch arena(1);
        auto bytes = arena.allocate(1);
        require(bytes.size() == 1, "Lifetime test could not allocate scratch");
        bytes.front() = std::byte{9};
        published = bytes;
        lease.emplace(arena.seal());
    }
    require(static_cast<bool>(*lease) && published.front() == std::byte{9}, "Owner destruction freed leased storage");
}
// End of Code Modification - Tests
}

int main()
{
    try {
        handles();
        scratch();
        leaseRetainsBacking();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
