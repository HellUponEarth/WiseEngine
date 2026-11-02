# Interface reservation only: Audio implementation belongs to later milestones.
add_library(Audio INTERFACE)
target_link_libraries(Audio INTERFACE Core)
