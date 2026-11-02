# WiseEngine 0.0.1 validation

## Acceptance result

**PASS: build and dependency baseline**, validated on 2026-11-02 in `F:\WiseEngine`. No application-shell, rendering or gameplay milestone is claimed.

| Check | Result | Local evidence |
|---|---|---|
| Clean Debug configure/build | PASS | `build/validation/20261102-214635/debug.log` |
| Clean Release configure/build | PASS | `build/validation/20261102-214635/release.log` |
| Debug CTest | 9/9 PASS | Clean log and `build/final-debug.log` |
| Release CTest | 9/9 PASS | Clean log and `build/final-release.log` |
| Fresh local Git source checkout | PASS | `build/fresh-checkout-configure.log`, `build/fresh-checkout-build.log` |
| Fresh-checkout executable | PASS | Release, version 0.0.1, 64-bit, C++ 202002 |
| Final bootstrap-cache configuration | PASS | Final Debug/Release configure, build and all tests repeated |
| Offline SDK/helper bootstrap | PASS | `build/offline-bootstrap.log`; a new local venv was also created from cached wheels during initial bring-up |
| SDK provenance | PASS | 25 SDK pins, 17 initialized submodules, 3 generator packages; `thirdparty/dependencies.installed.json` |
| Default executable startup/shutdown | PASS | Both configurations log their build identity and exit successfully |
| Recovery regressions | 3/3 PASS | ZIP extraction, payload corruption detection, unsafe-path rejection; part of CTest |

## Inputs and reproducibility

Toolchain: CMake 4.2.1, Ninja 1.12.0, MSVC 19.38.33145.0/VS2022 x64, Windows SDK 10.0.26100, Python 3.14.2. Host CPU: Ryzen 7 5800X, 16 logical processors.

Dependency lock SHA-256: `499bbf97a5ad18dd592de3f059d13d9afe872731d75cc7b533bbed3959ef4a93`.

Clean builds used new build directories without existing objects or libraries. The separate source probe was a real local Git clone of the committed engine source with a new build directory; it shared the already provenance-verified SDK inputs using directory junctions and built the App target. It did not repeat network downloads or the entire SDK suite in the clone. Later cache-isolation changes were verified in the final Debug/Release reconfiguration; C++ source was unchanged.

Reproducible means pinned inputs and repeatable successful builds. Byte-identical PE/PDB output has not been established. Build logs and generated reports stay local rather than entering public source control.

## Coverage

The nine CTest cases cover immutable build settings, version/configuration CLI output, invalid arguments, rejection of Core -> Input reverse links, the combined SDK ABI smoke test, DXIL and SPIR-V compilation, and recovery integrity. The SDK test executes Luau bytecode/native bindings, enkiTS jobs, representative data/resource operations and DX12/Vulkan factory calls without creating a graphics device or window.

## Recovery archive evidence

After committing/tagging this accepted source, `cmake/BackupMilestone.py --version 0.0.1` creates the local recovery ZIP. That command verifies every archived file against the embedded SHA-256 inventory before finalizing the archive and writes an archive SHA-256 sidecar. The archive manifest records its exact source commit. See `BACKUPS.md` for restore commands and the required toolchain/Python ABI. The completed archive and sidecar are the evidence for that post-commit step; they remain outside Git.

## Limits

No GPU frame/window, direct visual/control review, audio playback, TCP/UDP message transport, terrain streaming, foliage, ocean, game-physics integration, navigation or animation was accepted. Those are later increments. Optional SDK/tool executables were exposed but not separately compiled in this milestone's WiseEngine builds. SDK archive verification checks cached archives and installation markers rather than every extracted archive file; Git SDK tracked trees and recursive pins were checked for changes.
