#include "Core.h"

#include <limits>

namespace CE::Core {
// Code Modification - Core Timing: Keep ticks authoritative so floating-point errors do not accumulate.
// Scope: explicit simulation advancement. Performance: constant CPU work. Toggle: none.
bool SimulationClock::advance(std::uint64_t count) noexcept
{
    if (count > std::numeric_limits<std::uint64_t>::max() - ticks_) {
        return false;
    }
    ticks_ += count;
    return true;
}

std::uint64_t SimulationClock::ticks() const noexcept
{
    return ticks_;
}

double SimulationClock::seconds() const noexcept
{
    return static_cast<double>(ticks_) / TicksPerSecond;
}
// End of Code Modification - Core Timing
}
