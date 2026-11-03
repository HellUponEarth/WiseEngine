# Code Modification - Build System: Expose SDL event types through the snapshot without moving video ownership.
# Scope: Input target. Performance: fixed snapshot storage. Toggle: none.
add_library(Input STATIC Input.cpp Input.h)
target_include_directories(Input PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(Input PUBLIC ThirdParty::SDL)
wiseengine_target(Input)
# End of Code Modification - Build System
