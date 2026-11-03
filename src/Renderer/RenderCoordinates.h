#pragma once

#include "Math/WorldCoordinates.h"

namespace CE::Renderer {
// Code Modification - Rendering: Expose a GPU-friendly offset after subtracting the render origin in double precision.
// Scope: CPU coordinate bridge. Performance: on demand, no GPU work. Toggle: none.
struct RenderPosition {
    float x{};
    float y{};
    float z{};
};

std::optional<RenderPosition> toRenderPosition(Math::WorldPosition world, Math::WorldPosition origin) noexcept;
// End of Code Modification - Rendering
}
