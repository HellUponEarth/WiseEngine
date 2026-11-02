# Core must remain independent of platform-window, input, and rendering lifecycles.
configure_file(BuildVersion.h.in "${PROJECT_BINARY_DIR}/generated/Core/BuildVersion.h" @ONLY)
add_library(Core STATIC Core.h BuildInfo.h BuildInfo.cpp)
target_include_directories(Core PUBLIC "${PROJECT_SOURCE_DIR}/src" PRIVATE "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(Core PRIVATE Math ThirdParty::fmt ThirdParty::spdlog)
wiseengine_target(Core)
