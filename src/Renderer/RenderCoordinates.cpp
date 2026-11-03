#include "RenderCoordinates.h"

#include <glm/vec3.hpp>

namespace CE::Renderer {
// Code Modification - Rendering: Keep GLM private while preserving precision for distant world positions.
// Scope: coordinate conversion only. Performance: three double subtractions and float conversions. Toggle: none.
std::optional<RenderPosition> toRenderPosition(Math::WorldPosition world, Math::WorldPosition origin) noexcept
{
    if (!Math::isSupportedWorldPosition(world) || !Math::isSupportedWorldPosition(origin)) {
        return std::nullopt;
    }
    const glm::dvec3 worldVector(world.x, world.y, world.z);
    const glm::dvec3 originVector(origin.x, origin.y, origin.z);
    const glm::vec3 relative(worldVector - originVector);
    return RenderPosition{relative.x, relative.y, relative.z};
}
// End of Code Modification - Rendering
}
