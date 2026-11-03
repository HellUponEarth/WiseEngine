#include "PlatformInfo.h"

#include <boost/predef/architecture/x86.h>
#include <boost/predef/compiler/visualc.h>
#include <boost/predef/os/windows.h>
#include <boost/predef/other/endian.h>
#include <bit>
#include <climits>

namespace CE::App {
// Code Modification - Platform: Fail compilation when target detection disagrees with the supported ABI.
// Scope: Windows x64 build contract. Performance: compile-time checks. Toggle: none.
static_assert(BOOST_OS_WINDOWS && BOOST_ARCH_X86_64 && BOOST_COMP_MSVC && BOOST_ENDIAN_LITTLE_BYTE);
static_assert(std::endian::native == std::endian::little);
static_assert(CHAR_BIT == 8 && sizeof(void*) == 8);

PlatformInfo getPlatformInfo() noexcept
{
    return {BOOST_OS_WINDOWS_NAME, BOOST_ARCH_X86_64_NAME, BOOST_COMP_MSVC_NAME, "little",
        static_cast<std::uint32_t>(sizeof(void*) * CHAR_BIT)};
}
// End of Code Modification - Platform
}
