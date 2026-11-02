# Math is an interface reservation until the coordinate/math milestone; it has no dependencies.
add_library(Math INTERFACE)
target_include_directories(Math INTERFACE "${PROJECT_SOURCE_DIR}/src")
