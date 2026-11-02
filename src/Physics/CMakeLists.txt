# Interface reservation only: Physics implementation belongs to later milestones.
add_library(Physics INTERFACE)
target_link_libraries(Physics INTERFACE Math Core)
