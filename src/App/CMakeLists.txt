# Code Modification - Build System: Keep SDL video and spdlog lifecycle diagnostics in App.
# Scope: application shell. Performance: no extra runtime work beyond the shell. Toggle: --quiet-title.
add_executable(App Main.cpp ApplicationShell.cpp ApplicationShell.h)
set_target_properties(App PROPERTIES OUTPUT_NAME WiseEngine)
target_link_libraries(App PRIVATE Core Input ThirdParty::spdlog)
wiseengine_target(App)
# End of Code Modification - Build System
