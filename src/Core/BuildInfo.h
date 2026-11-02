#pragma once
#include <cstdint>
#include <string_view>
#include "Core.h"

namespace CE::Core {
// Code Modification - Core: Expose the actual build identity so milestone artifacts can be diagnosed.
// Scope: immutable baseline metadata. Performance: no allocation or GPU work. Toggle: none.
struct BuildInfo {
    std::string_view version;
    std::string_view configuration;
    std::string_view compiler;
    std::uint32_t pointerBits;
    std::uint32_t cppStandard;
};
[[nodiscard]] BuildInfo getBuildInfo() noexcept;
void logBuildInfo();
// End of Code Modification - Core
}
