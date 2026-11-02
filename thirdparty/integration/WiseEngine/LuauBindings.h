#pragma once
// Code Modification - Scripting: Centralize the pinned Luau/LuaBridge include order and confine one upstream MSVC diagnostic.
// Scope: SDK compatibility header only. Performance: no runtime cost. Toggle: none; engine warning checks remain enabled.
#include <lua.h>
#include <lualib.h>
#include <Luau/Compiler.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
#include <LuaBridge/LuaBridge.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
// End of Code Modification - Scripting
