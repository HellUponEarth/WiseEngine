#include "ElapsedClock.h"

#include <SDL3/SDL_timer.h>

namespace CE::App {
// Code Modification - Platform Timing: Subtract integer samples before converting to seconds.
// Scope: SDL elapsed-time adapter, usable before video initialization. Performance: one frequency lookup per conversion. Toggle: none.
ElapsedClock::Counter ElapsedClock::now() noexcept
{
    return SDL_GetPerformanceCounter();
}

ElapsedClock::Counter ElapsedClock::frequency() noexcept
{
    return SDL_GetPerformanceFrequency();
}

std::optional<double> ElapsedClock::secondsBetween(Counter begin, Counter end) noexcept
{
    const auto ticksPerSecond = frequency();
    if (end < begin || ticksPerSecond == 0) {
        return std::nullopt;
    }
    return static_cast<double>(end - begin) / static_cast<double>(ticksPerSecond);
}
// End of Code Modification - Platform Timing
}
