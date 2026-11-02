# Interface reservation only: AssetManager implementation belongs to later milestones.
add_library(AssetManager INTERFACE)
target_link_libraries(AssetManager INTERFACE Core Math)
