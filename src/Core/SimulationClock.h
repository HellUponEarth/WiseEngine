#pragma once

#include <cstdint>

namespace CE::Core {
// Code Modification - Core Timing: Let the simulation owner advance time explicitly at a fixed step.
// Scope: simulation counter independent of elapsed time. Performance: one 64-bit counter. Toggle: advance only when stepping.
class SimulationClock {
public:
    static constexpr std::uint32_t TicksPerSecond = 60;

    bool advance(std::uint64_t count = 1) noexcept;
    std::uint64_t ticks() const noexcept;
    double seconds() const noexcept;

private:
    std::uint64_t ticks_{};
};
// End of Code Modification - Core Timing
}
