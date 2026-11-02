# The baseline executable proves linkage; the interactive shell begins at 0.0.2.
add_executable(App Main.cpp)
set_target_properties(App PROPERTIES OUTPUT_NAME WiseEngine)
target_link_libraries(App PRIVATE Core)
wiseengine_target(App)
