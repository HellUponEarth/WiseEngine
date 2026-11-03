#include "App/ElapsedClock.h"
#include "App/PlatformInfo.h"
#include "Core/Core.h"
#include "Math/WorldCoordinates.h"
#include "Renderer/RenderCoordinates.h"

#include <SDL3/SDL_timer.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
// Code Modification - Tests: Exercise precision boundaries and independent time domains.
// Scope: test executable. Performance: one short SDL wait. Toggle: WISEENGINE_BUILD_TESTS.
bool check(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool coordinates()
{
    using CE::Math::CellCoordinate;
    using CE::Math::CellLimit;
    using CE::Math::CellSize;
    using CE::Math::WorldLimit;
    using CE::Math::joinWorldPosition;
    using CE::Math::splitWorldPosition;

    const auto point = splitWorldPosition({-1024.25, -1024.0, -0.25});
    const auto world = point ? joinWorldPosition(*point) : std::nullopt;
    const bool negative = check(point && point->cell == CellCoordinate{-2, -1, -1} &&
        point->local.x == 1023.75 && point->local.y == 0.0 && point->local.z == 1023.75 &&
        world && world->x == -1024.25 && world->y == -1024.0 && world->z == -0.25,
        "Negative coordinates must floor to their containing cells");

    const double maxWorld = std::nextafter(WorldLimit, 0.0);
    const auto bounds = splitWorldPosition({-WorldLimit, maxWorld, CellSize});
    const auto reconstructed = bounds ? joinWorldPosition(*bounds) : std::nullopt;
    const bool extremes = check(bounds && bounds->cell == CellCoordinate{-CellLimit, CellLimit - 1, 1} &&
        reconstructed && reconstructed->x == -WorldLimit && reconstructed->y == maxWorld &&
        reconstructed->z == CellSize, "Valid extremes and exact positive edges must round trip");

    const auto tiny = splitWorldPosition({-std::numeric_limits<double>::denorm_min(), 0.0, 0.0});
    const auto tinyWorld = tiny ? joinWorldPosition(*tiny) : std::nullopt;
    const auto upper = joinWorldPosition({{CellLimit - 1, 0, 0}, {std::nextafter(CellSize, 0.0), 0.0, 0.0}});
    const bool rounding = check(tiny && tiny->cell.x == -1 && tiny->local.x < CellSize &&
        tinyWorld && tinyWorld->x < 0.0 && upper && upper->x == maxWorld,
        "Rounding must preserve cell membership");

    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const bool invalid = check(!splitWorldPosition({WorldLimit, 0.0, 0.0}) &&
        !splitWorldPosition({0.0, std::nextafter(-WorldLimit, -inf), 0.0}) &&
        !splitWorldPosition({0.0, inf, 0.0}) && !splitWorldPosition({0.0, 0.0, nan}) &&
        !joinWorldPosition({{CellLimit, 0, 0}, {}}) && !joinWorldPosition({{0, -CellLimit - 1, 0}, {}}) &&
        !joinWorldPosition({{}, {CellSize, 0.0, 0.0}}) && !joinWorldPosition({{}, {0.0, -1.0, 0.0}}) &&
        !joinWorldPosition({{}, {0.0, 0.0, nan}}), "Invalid coordinates must be rejected");

    constexpr double base = 1'000'000'000'000.0;
    const auto render = CE::Renderer::toRenderPosition({base + 0.25, -base - 0.5, 3.0}, {base, -base, 1.0});
    const bool relative = check(render && render->x == 0.25f && render->y == -0.5f && render->z == 2.0f &&
        !CE::Renderer::toRenderPosition({}, {WorldLimit, 0.0, 0.0}),
        "Render offsets must subtract in double precision before narrowing");
    return negative && extremes && rounding && invalid && relative;
}

bool clocks()
{
    using CE::App::ElapsedClock;
    CE::Core::SimulationClock simulation;
    const auto before = ElapsedClock::now();
    SDL_Delay(5);
    const auto after = ElapsedClock::now();
    const auto elapsed = ElapsedClock::secondsBetween(before, after);
    const auto frequency = ElapsedClock::frequency();
    constexpr auto maxCounter = std::numeric_limits<std::uint64_t>::max();
    const bool independent = check(frequency > 0 && elapsed && *elapsed > 0.0 &&
        ElapsedClock::secondsBetween(maxCounter - frequency, maxCounter) == 1.0 &&
        ElapsedClock::secondsBetween(before, before) == 0.0 && !ElapsedClock::secondsBetween(after, before) &&
        simulation.ticks() == 0 && simulation.seconds() == 0.0, "Elapsed time must be independent of simulation time");
    const bool fixed = check(simulation.advance(60) && simulation.ticks() == 60 && simulation.seconds() == 1.0 &&
        simulation.advance(maxCounter - 60) && !simulation.advance() && simulation.ticks() == maxCounter,
        "Fixed ticks must advance explicitly and refuse overflow without mutation");
    return independent && fixed;
}
// End of Code Modification - Tests
}

int main()
{
    const auto platform = CE::App::getPlatformInfo();
    const bool abi = check(platform.os == "Microsoft Windows" && platform.architecture == "Intel x86-64" &&
        platform.compiler == "Microsoft Visual C/C++" && platform.byteOrder == "little" && platform.pointerBits == 64,
        "Platform report must match the supported Windows x64 MSVC ABI");
    const bool world = coordinates();
    const bool time = clocks();
    return abi && world && time ? 0 : 1;
}
