#include "Core/Core.h"
#include <iostream>

// Code Modification - Tests: Fail when target settings diverge from the milestone contract.
// Scope: standalone baseline check. Performance: test-only. Toggle: WISEENGINE_BUILD_TESTS.
int main()
{
    const auto info = CE::Core::getBuildInfo();
    const bool valid = info.version == WISEENGINE_EXPECTED_VERSION && info.pointerBits == 64 && info.cppStandard >= 202002L
        && (info.configuration == "Debug" || info.configuration == "Release" || info.configuration == "RelWithDebInfo")
        && info.compiler.starts_with("MSVC ");
    if (!valid) { std::cerr << "Build identity does not meet the configured contract.\n"; }
    return valid ? 0 : 1;
}
// End of Code Modification - Tests
