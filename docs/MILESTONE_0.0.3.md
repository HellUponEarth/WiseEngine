# WiseEngine 0.0.3

## Section 1 – C++ Code

- `src/App/PlatformInfo.h` exposes the compiler target's OS, architecture, compiler family, byte order and pointer width. Boost.Predef and standard compile-time checks enforce the Windows x64 MSVC little-endian ABI.
- `src/Math/WorldCoordinates.h` defines double-precision XYZ world positions and signed `std::int64_t` XYZ cell indices. Each cell is 1,024 world units across. Each axis accepts world coordinates in `[-2^52, 2^52)` and cell indices in `[-2^42, 2^42)`. Split uses floor and returns a local offset in `[0, 1024)`. Invalid, non-finite and out-of-range input returns an empty optional.
- `src/Renderer/RenderCoordinates.h` converts a world position and an explicit render origin into a three-component float offset. The implementation uses GLM privately and subtracts in double precision before narrowing.
- `src/App/ElapsedClock.h` wraps SDL performance-counter samples and converts integer differences to elapsed seconds. Both samples must come from the same process and be taken in order. `src/Core/SimulationClock.h` counts explicit 60 Hz steps and refuses overflow. Its seconds value is derived from the tick count.

## Section 2 – Build Configuration

Math remains SDK-free. Core continues to depend only on Math and fmt/spdlog. Renderer links Math and Core and uses GLM privately. App owns the Boost.Predef and SDL clock adapters. Tests build with `WISEENGINE_BUILD_TESTS`, enabled by the build script's `-Tests` switch.

## Section 3 – Scripts

Use the existing scripts from the engine root. Verify installed SDKs with `python cmake/VerifyThirdParty.py`. Configure, build and test with `./build.ps1 -Config Debug -Target all -Tests` and repeat with `-Config Release`. After committing and tagging, create the recovery ZIP with `python cmake/BackupMilestone.py --version 0.0.3`.

## Section 4 – Documentation

Cell ranges are half-open. For example, −0.25 belongs to cell −1 with local offset 1023.75. At rounding boundaries the conversions preserve cell membership. A position infinitesimally below zero may round to a nearby negative value when reconstructed, and arbitrary cell-local offsets may lose precision when converted to absolute coordinates at extreme distances. The float render offset is intended for positions near the chosen render origin. It cannot recover precision already lost in absolute double coordinates.

Coordinate and timing work runs only on request and uses no heap allocation. The platform diagnostic is opt-in via `--platform-info`. The SDL event loop continues to wait for input. A future simulation owner will decide when to issue steps, how to handle pauses, and how to schedule catch-up. The render bridge prepares CPU values for a future GPU integration.

Debug and Release incremental builds each passed all 12 CTest cases. SDK provenance verification passed for 25 packages, pinned submodules, and three local generator packages. The platform probe succeeded with an invalid SDL video driver and reported a 10 MHz counter on the test machine.

## Section 5 – Usage Examples

```powershell
.\build\debug\bin\WiseEngine.exe --platform-info
```

From C++, call `CE::Math::splitWorldPosition({-0.25, 0.0, 0.0})` and check the optional before using its cell. For a nearby render object, call `CE::Renderer::toRenderPosition(objectPosition, cameraOrigin)` and check the result before passing its components to the renderer. For elapsed timing, take two samples using `CE::App::ElapsedClock::now()` and pass them to `secondsBetween`. Advance `CE::Core::SimulationClock` explicitly for each simulated step.
