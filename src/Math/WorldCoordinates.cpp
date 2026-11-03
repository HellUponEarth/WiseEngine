#include "WorldCoordinates.h"


#include <cmath>

namespace CE::Math {
namespace {
// Code Modification - Math: Keep the cell interval half-open at boundaries on either side of zero.
// Scope: axis conversion. Performance: constant arithmetic per axis. Toggle: none.
bool supportedAxis(double value) noexcept
{
    return value >= -WorldLimit && value < WorldLimit;
}

std::int64_t cellFor(double value) noexcept
{
    // Division by a power of two can underflow a tiny negative value to signed zero.
    if (value < 0.0 && value > -CellSize) {
        return -1;
    }
    return static_cast<std::int64_t>(std::floor(value / CellSize));
}

double localFor(double value, std::int64_t cell) noexcept
{
    const auto offset = value - static_cast<double>(cell) * CellSize;
    // Cancellation near zero can round the local offset up to the excluded upper edge.
    return offset < CellSize ? offset : std::nextafter(CellSize, 0.0);
}

bool validAxis(std::int64_t cell, double local) noexcept
{
    return cell >= -CellLimit && cell < CellLimit && local >= 0.0 && local < CellSize;
}

double joinAxis(std::int64_t cell, double local) noexcept
{
    const auto base = static_cast<double>(cell) * CellSize;
    const auto upper = base + CellSize;
    // Preserve the selected cell even when the world-space sum rounds to its upper edge.
    const auto value = base + local;
    return value < upper ? value : std::nextafter(upper, base);
}
// End of Code Modification - Math
}

// Code Modification - Math: Validate before narrowing or reconstructing coordinates.
// Scope: public grid conversions. Performance: constant CPU work per conversion. Toggle: none.
bool isSupportedWorldPosition(WorldPosition position) noexcept
{
    return supportedAxis(position.x) && supportedAxis(position.y) && supportedAxis(position.z);
}

std::optional<CellPosition> splitWorldPosition(WorldPosition position) noexcept
{
    if (!isSupportedWorldPosition(position)) {
        return std::nullopt;
    }
    const CellCoordinate cell{cellFor(position.x), cellFor(position.y), cellFor(position.z)};
    return CellPosition{cell, {localFor(position.x, cell.x), localFor(position.y, cell.y),
        localFor(position.z, cell.z)}};
}

std::optional<WorldPosition> joinWorldPosition(CellPosition position) noexcept
{
    const auto [cell, local] = position;
    if (!validAxis(cell.x, local.x) || !validAxis(cell.y, local.y) || !validAxis(cell.z, local.z)) {
        return std::nullopt;
    }
    return WorldPosition{joinAxis(cell.x, local.x), joinAxis(cell.y, local.y), joinAxis(cell.z, local.z)};
}
// End of Code Modification - Math
}
