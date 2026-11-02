# Interface reservation only: Renderer implementation belongs to later milestones.
add_library(Renderer INTERFACE)
target_link_libraries(Renderer INTERFACE Math Core)
