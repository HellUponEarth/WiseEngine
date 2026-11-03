# Code Modification - Build System: Keep SDL video, lifecycle diagnostics and the jobs probe in App.
# Scope: application shell. Performance: no extra runtime work beyond the shell. Toggles: --quiet-title and --job-smoke.
add_executable(App Main.cpp ApplicationShell.cpp ApplicationShell.h ElapsedClock.cpp ElapsedClock.h PlatformInfo.cpp PlatformInfo.h)
set_target_properties(App PROPERTIES OUTPUT_NAME WiseEngine)
target_link_libraries(App PRIVATE Core Input Jobs ThirdParty::spdlog ThirdParty::Boost ThirdParty::SDL)
wiseengine_target(App)
# End of Code Modification - Build System
