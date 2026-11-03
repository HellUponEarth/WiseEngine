#pragma once

#include <cstdint>
#include <string_view>

namespace CE::App {
// Code Modification - Platform: Report target properties independently of SDL video and the host OS version.
// Scope: compile-target diagnostics. Performance: constant-time query. Toggle: --platform-info.
struct PlatformInfo {
    std::string_view os;
    std::string_view architecture;
    std::string_view compiler;
    std::string_view byteOrder;
    std::uint32_t pointerBits;
};

PlatformInfo getPlatformInfo() noexcept;
// End of Code Modification - Platform
}
