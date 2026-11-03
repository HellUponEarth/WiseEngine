# WiseEngine 0.0.5

## Section 1 – C++ Code

- `src/Core/FrameScratch.h` exposes a fixed-capacity byte arena for a single owner and a single consumer. Allocations bump the cursor with alignment up to 64 bytes. Zero-byte, invalid-alignment, oversized, and sealed requests return an empty span without consuming capacity.
- `src/Core/HandleTable.h` exposes a typed, fixed-capacity table using `std::vector` and `std::optional` slots. A handle carries a process-local table identity, index, and generation. Lookup rejects foreign and stale handles. A slot retires at generation exhaustion. Object construction can throw without consuming a slot.
- `src/Jobs/JobSystem.cpp` uses Boost.Container `static_vector` for the existing bounded list of active CPU graphs. The list uses inline storage and keeps the 64-entry ceiling. Graph allocations and worker scheduling retain their existing ownership rules.

## Section 2 – Build Configuration

Frame scratch and handle tables are part of Core and use the C++ standard library. Jobs links the pinned Boost headers privately for its inline collection. `WISEENGINE_BUILD_TESTS` enables the Core memory contract test alongside the Jobs tests.

## Section 3 – Scripts

Run `python cmake/VerifyThirdParty.py` and `./build.ps1 -Config Debug -Target all -Tests` from the engine root. Repeat the build with `-Config Release`. After committing and tagging, use `python cmake/BackupMilestone.py --version 0.0.5` to create and verify the recovery ZIP.

## Section 4 – Documentation

- **Scratch ownership.** The owner writes its allocations, calls `seal()`, then hands the move-only lease and read-only spans to one consumer through an existing synchronized handoff. The owner stops using writable spans at seal. The lease keeps the storage alive even if the arena owner is destroyed. The consumer drops its spans before releasing the lease. `tryReset()` returns false while leased. A successful reset invalidates all prior spans and reuses the byte capacity without clearing it. Use one arena per in-flight frame if frames overlap. Callers manage any object construction and destruction separately.
- **Handle invalidation.** Pointers returned by `get()` remain stable until their individual object is erased or the table is destroyed. Re-resolve the handle after any operation that could erase the object. Tables are owner-thread containers and their handles are process-local. `capacity()` reports original slot count, including any retired slots.
- **Collection invalidation.** The Jobs list stays within fixed inline storage. Removing completed batches compacts the list, so iterators and references at or after the erased position must be discarded. The list is private and its iterators never leave the owner.
- **Cost and control.** Scratch capacity is chosen at construction. A nonempty arena allocates backing storage once, plus its lifetime control state. Lease handoff uses an atomic flag and shared ownership. Tables allocate their slots once and perform constant-time operations. Jobs reserves inline pointer storage per scheduler instance. Runtime callers opt in by constructing these services.

Debug and Release incremental builds each passed all 15 CTest cases. The memory and Jobs contract tests each passed 100 consecutive runs per configuration. SDK provenance verification passed for 25 pinned packages, their submodules, and three local generator packages.

## Section 5 – Usage Examples

Construct `CE::Core::FrameScratch frame(64 * 1024)` and call `frame.allocate(bytes, 16)`. Write only when the span is nonempty. Move the result of `frame.seal()` with the read-only span to the consumer. Reuse the arena only when `frame.tryReset()` returns true.

Construct `CE::Core::HandleTable<Chunk> chunks(128)`. Check the optional returned by `chunks.emplace(/* constructor arguments */)`. Resolve a handle with `chunks.get(handle)` and check the pointer before use. Call `chunks.erase(handle)` when unloading the chunk.
