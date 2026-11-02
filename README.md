# WiseEngine

WiseEngine is a C++20 game-engine project using Diligent for rendering and Luau for gameplay scripting. **Version 0.0.1 is the build and dependency baseline.** It currently runs command-line diagnostics; it does not render a scene or run a game.

## Build

Requires Windows x64, Visual Studio 2022 Desktop development with C++, Windows SDK 10.0.19041+ (10.0.26100 tested), CMake 3.31+, Ninja, Python 3.12+, and Git. No commercial SDK is required.

```powershell
python cmake/FetchThirdParty.py --jobs 4
python cmake/VerifyThirdParty.py
.\build.ps1 -Config Debug -Target all -Tests
.\build.ps1 -Config Release -Target all -Tests
.\build\debug\bin\WiseEngine.exe --build-info
```

The build script imports the VS2022 x64 environment into its own process. Direct `cmake --preset debug`, `cmake --build --preset debug` and `ctest --preset debug` commands work in an MSVC developer shell. CMake consumes local SDKs and does not fetch them. Initial bootstrap requires network access; downloads are pinned by Git commit or SHA-256. Downloaded third-party sources are not checked into this repository.

## Milestones and recovery

- [0.0.1 scope, implementation and usage](docs/MILESTONE_0.0.1.md)
- [Build validation](docs/VALIDATION_0.0.1.md)
- [SDK inventory and module links](docs/THIRD_PARTY.md)
- [Architecture and ownership](docs/ARCHITECTURE.md)
- [Hardware and performance targets](docs/HARDWARE_AND_BUDGETS.md)
- [Roadmap to 0.1](docs/ROADMAP.md)
- [Milestone backups and restore](docs/BACKUPS.md)

After a milestone passes validation, commit/tag the source, run `python cmake/BackupMilestone.py --version <version>`, and push the branch/tag. The ZIP includes source, installed SDKs, their local Git metadata, cached SDK archives and generator wheels. It omits previous backups, build outputs, the engine Git database and the Python venv. Every archived file is hash-verified; existing backups are never overwritten.

## Status and licensing

Only Core build metadata and the diagnostic executable are implemented. The other module targets reserve dependency boundaries for later milestones. SDK smoke tests prove linkage and selected library operations; they do not implement engine backends. Terrain, dense foliage, ocean simulation, interactive input, and GPU rendering remain roadmap work.

No license has been selected for WiseEngine's own source. Public visibility does not grant a general redistribution license. SDKs remain under their upstream licenses; preserve applicable copyright and third-party notices when distributing them. See [third-party notices](THIRD_PARTY_NOTICES.md).
