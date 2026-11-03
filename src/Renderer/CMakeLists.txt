# Code Modification - Build System: Compile and embed the baseline HLSL with the pinned DXC.
# Scope: Renderer target. Performance: offline shader compilation. Toggle: --no-render bypasses runtime GPU work.
set(bringup_shader "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/BringUp.hlsl")
set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_command(
    OUTPUT "${shader_output}/BringUpVS.h" "${shader_output}/BringUpPS.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${shader_output}"
    COMMAND ThirdParty::dxc -T vs_6_0 -E VSMain -WX -Fh "${shader_output}/BringUpVS.h" -Vn BringUpVS "${bringup_shader}"
    COMMAND ThirdParty::dxc -T ps_6_0 -E PSMain -WX -Fh "${shader_output}/BringUpPS.h" -Vn BringUpPS "${bringup_shader}"
    DEPENDS "${bringup_shader}" ThirdParty::dxc
    VERBATIM)
add_library(Renderer STATIC RenderCoordinates.cpp RenderCoordinates.h Dx12Scene.cpp Dx12Scene.h
    "${shader_output}/BringUpVS.h" "${shader_output}/BringUpPS.h")
target_include_directories(Renderer PRIVATE "${shader_output}")
target_link_libraries(Renderer PUBLIC Math Core PRIVATE ThirdParty::glm ThirdParty::Diligent)
wiseengine_target(Renderer)
# End of Code Modification - Build System
