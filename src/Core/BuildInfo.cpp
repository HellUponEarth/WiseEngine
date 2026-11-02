#include "BuildInfo.h"
#include "Core/BuildVersion.h"
#include <spdlog/spdlog.h>

namespace CE::Core {
// Code Modification - Core: Report compiler-proven settings instead of assuming a preset was honored.
// Scope: baseline diagnostics. Performance: one optional log allocation at startup. Toggle: callers may skip logBuildInfo.
static_assert(__cplusplus >= 202002L, "WiseEngine requires C++20");
static_assert(sizeof(void*) == 8, "WiseEngine requires x64");
BuildInfo getBuildInfo() noexcept
{
    return {WISEENGINE_VERSION, WISEENGINE_CONFIGURATION, WISEENGINE_COMPILER,
        static_cast<std::uint32_t>(sizeof(void*) * 8), static_cast<std::uint32_t>(__cplusplus)};
}
void logBuildInfo()
{
    const auto info = getBuildInfo();
    spdlog::info("WiseEngine {} | {} | {} | {}-bit | C++ {}",
        info.version, info.configuration, info.compiler, info.pointerBits, info.cppStandard);
}
// End of Code Modification - Core
}
