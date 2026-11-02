# Interface reservation only: Network implementation belongs to later milestones.
add_library(Network INTERFACE)
target_link_libraries(Network INTERFACE Core)
