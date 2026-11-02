# Interface reservation only: Input implementation belongs to later milestones.
add_library(Input INTERFACE)
target_link_libraries(Input INTERFACE Core)
