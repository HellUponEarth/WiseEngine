#pragma once

#include <cstdint>
#include <optional>

namespace CE::App {
// Code Modification - Platform Timing: Keep SDL counter samples separate from simulation ticks.
// Scope: elapsed time in one process. Performance: counter reads only on demand. Toggle: none.
class ElapsedClock {
public:
    using Counter = std::uint64_t;

    static Counter now() noexcept;
    static Counter frequency() noexcept;
    static std::optional<double> secondsBetween(Counter begin, Counter end) noexcept;
};
// End of Code Modification - Platform Timing
}
