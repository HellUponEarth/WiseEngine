# Code Modification - Build System: Keep GLM private to the render-coordinate adapter.
# Scope: CPU-only Renderer target. Performance: no startup or GPU work. Toggle: none.
add_library(Renderer STATIC RenderCoordinates.cpp RenderCoordinates.h)
target_link_libraries(Renderer PUBLIC Math Core PRIVATE ThirdParty::glm)
wiseengine_target(Renderer)
# End of Code Modification - Build System
