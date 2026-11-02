# Section 1 - C++ Code

## Engine changes

Version 0.0.1 establishes a clean WiseEngine baseline at `F:\WiseEngine`. `F:\CustomEngine` is a separate preserved project. The existing pinned dependency selection was reused; unfinished application-shell source was not imported.

- `src/Core/BuildInfo.h` and `.cpp`: immutable version, configuration, compiler, pointer width and effective C++ standard. Compile-time assertions require C++20 and 64-bit pointers.
- `src/Core/Core.h`: public Core umbrella. Existing `CE::<Module>` namespace convention is retained.
- `src/App/Main.cpp`: bounded console diagnostics with `--version`, `--build-info`, `--help`, and nonzero errors for invalid arguments.
- `src/Math` and other non-Core modules contain explicitly documented interface reservations, not placeholder implementations advertised as functional backends.

No window, render thread, input lifecycle, physics world, network transport, or Luau game API is added at this milestone. The next increment is 0.0.2 application-shell implementation.

# Section 2 - Build Configuration

`CMakeLists.txt` declares version 0.0.1, C++20, no language extensions, dynamic MSVC CRT, and project-only `/W4 /WX /permissive-` warnings. `/Zc:__cplusplus` makes compiler standard metadata accurate. SDK warning policies remain upstream-owned.

`CMakePresets.json` provides Debug, Release and RelWithDebInfo Ninja configurations. `cmake/ModuleBoundaries.cmake` rejects forbidden direct engine links; its negative test constructs Core -> Input -> Core and verifies the expected configure error. Math has no dependencies; Core links only Math and fmt/spdlog. App owns orchestration. A dependency's PUBLIC or PRIVATE visibility alone does not prevent static-library cycles.

`thirdparty/CMakeLists.txt` maps the lockfile sources to `ThirdParty::` targets and keeps downstream ABI choices consistent. Downloaded upstream sources remain unmodified. SDK links are selected per implementation; baseline App links Core only. The separate smoke executable links the selected SDK stack together.

## Performance toggles

| CMake option | Default | Purpose and cost |
|---|---|---|
| WISEENGINE_BUILD_TESTS | Debug ON; others OFF | Test compilation/runtime only; no shipping execution. |
| WISEENGINE_ENABLE_VULKAN | ON | Additional SDK compile and binary cost; OFF retains DX12. |
| WISEENGINE_ENABLE_PROFILING | OFF | Tracy CPU, memory, worker-thread and network cost when enabled. |
| WISEENGINE_JOLT_DOUBLE_PRECISION | ON | Double physics positions; increased CPU/memory cost; rebuild all consumers when changed. |
| WISEENGINE_BUILD_ASSET_TOOLS | ON | Exposes explicitly built tools; does not add them to the default build. |
| WISEENGINE_ENABLE_FUTURE_SDKS | OFF | Exposes ozz/Recast; additional build cost when requested. |

There is no frame loop or rendering cost to tune at 0.0.1. `--version` and `--build-info` skip startup logging. SDK tests and logging remain separate from the inexpensive metadata query.

# Section 3 - Scripts

- `build.ps1`: official incremental configure/build/test entry point; supports `-BuildDirectory build/<name>` for isolated validation. Only explicit `-Target clean` deletes the selected build tree after path checks.
- `cmake/FetchThirdParty.py`: exact SDK pins, cached archives, local Python generator environment. `--offline` rebuilds that environment from a complete backup without downloading packages.
- `cmake/VerifyThirdParty.py`: offline repository revisions, origins, staged/unstaged tracked edits, recursive Diligent submodule pins, archive checksums and generator package/wheel verification.
- `cmake/ValidateMilestone.ps1`: provenance check plus clean Debug/Release builds, tests and build identity; writes logs and JSON under a unique `build/validation/<timestamp>` directory.
- `cmake/BackupMilestone.py`: exclusive ZIP creation from a committed source state, complete inventory, per-file SHA-256 verification, archive SHA-256 sidecar. Refuses symbolic links/junctions and unsafe archive paths.

# Section 4 - Documentation

The acceptance gate is a fresh source checkout with pinned dependencies, successful clean Debug/Release builds, enforced module boundaries, and documented hardware, budgets, commands and 0.1 scope. Reproducibility here means identical pinned inputs and a repeatable successful build; byte-for-byte PE/PDB output identity is not claimed.

See `VALIDATION_0.0.1.md` for measured results and limits. Source/API/SDK tests do not establish visual correctness or frame-rate targets. SDK archive integrity checks verify downloaded archives and their marker metadata; they do not hash every extracted SDK file. The recovery ZIP verifies every file it actually archives.

## File placement and dependency instructions

Add engine classes as matching `.h`/`.cpp` under `src/<Module>` and list them in that module's CMake target. Use `CE::<Module>` namespaces, `#pragma once`, and the module umbrella header. Keep SDK adapters under `thirdparty/integration` and downloaded SDK trees read-only. New engine-module edges require an explicit architecture decision and an update to the module policy, not a reverse include or hidden target link. Gameplay code will belong under a dedicated project code folder when that milestone begins.

# Section 5 - Usage Examples

```powershell
.\build.ps1 -Config Debug -Target all -Tests -Jobs 6
.\build.ps1 -Config Release -Target all -Tests -Jobs 6
.\build\debug\bin\WiseEngine.exe --version
.\cmake\ValidateMilestone.ps1 -Jobs 6
python cmake/BackupMilestone.py --version 0.0.1
```

Use `-CMakeArgs @('-DWISEENGINE_ENABLE_VULKAN=OFF')` to select the cheaper DX12-only SDK build. Restore the default with `-CMakeArgs @('-DWISEENGINE_ENABLE_VULKAN=ON')`. Profiling stays OFF unless explicitly enabled using `-DWISEENGINE_ENABLE_PROFILING=ON`.
