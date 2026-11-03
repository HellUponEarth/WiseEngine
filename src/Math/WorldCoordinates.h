#pragma once

#include <cstdint>
#include <optional>

namespace CE::Math {
// Code Modification - Math: Give streaming and rendering a shared signed world grid.
// Scope: coordinate values and checked conversions. Performance: on demand, no allocation. Toggle: none.
struct WorldPosition {
    double x{};
    double y{};
    double z{};
};

struct CellCoordinate {
    std::int64_t x{};
    std::int64_t y{};
    std::int64_t z{};

    bool operator==(const CellCoordinate&) const = default;
};

struct CellPosition {
    CellCoordinate cell;
    WorldPosition local;
};

inline constexpr double CellSize = 1024.0;
inline constexpr std::int64_t CellLimit = std::int64_t{1} << 42;
inline constexpr double WorldLimit = static_cast<double>(CellLimit) * CellSize;

bool isSupportedWorldPosition(WorldPosition position) noexcept;
std::optional<CellPosition> splitWorldPosition(WorldPosition position) noexcept;
std::optional<WorldPosition> joinWorldPosition(CellPosition position) noexcept;
// End of Code Modification - Math
}
