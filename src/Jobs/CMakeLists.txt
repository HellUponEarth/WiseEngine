# Code Modification - Build System: Isolate SDL and enkiTS behind the engine-owned scheduler API.
# Scope: Jobs module. Performance: no worker startup until construction. Toggle: caller-owned lifetime.
add_library(Jobs STATIC JobSystem.cpp JobSystem.h)
target_include_directories(Jobs PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(Jobs PRIVATE ThirdParty::SDL ThirdParty::enkiTS)
wiseengine_target(Jobs)
# End of Code Modification - Build System
