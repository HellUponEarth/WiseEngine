# Code Modification - Build System: Compile checked coordinates without any SDK dependency.
# Scope: Math target. Performance: no startup work. Toggle: none.
add_library(Math STATIC WorldCoordinates.cpp WorldCoordinates.h)
target_include_directories(Math PUBLIC "${PROJECT_SOURCE_DIR}/src")
wiseengine_target(Math)
# End of Code Modification - Build System
