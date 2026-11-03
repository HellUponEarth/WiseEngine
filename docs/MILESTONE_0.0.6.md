# WiseEngine 0.0.6

## Section 1 – C++ Code

`src/AssetManager/FileSystem.h` exposes one process-wide VFS session. Mount loose directories or ZIP archives and read them through the same logical path. Mounts append by default. Pass `MountPriority::First` to give an override precedence. Missing files, invalid logical names, bad mounts, and over-limit reads return structured errors.

Save storage has a separate virtual root and write directory. Call `configureUserSaves()` for the OS-provided per-user location or `configureSaves(absolutePath)` for trusted tooling and tests. Saves create parent directories on demand. The service rejects duplicate mounts and overlapping native content and save roots.

## Section 2 – Build Configuration

AssetManager privately links the pinned static PhysicsFS target. App links AssetManager for the headless tool. The regular interactive startup opens no VFS session. `WISEENGINE_BUILD_TESTS` enables the filesystem integration test.

## Section 3 – Scripts

Run `python cmake/VerifyThirdParty.py` from the engine root and build with `./build.ps1 -Config Debug -Target all -Tests`, then repeat with `-Config Release`. CTest runs `tests/FileSystemFixture.py` to generate and remove temporary loose and deflated ZIP fixtures. After committing and tagging, use `python cmake/BackupMilestone.py --version 0.0.6` to create and verify the recovery ZIP.

## Section 4 – Documentation

- Logical paths are relative and slash-separated. Components allow ASCII letters, digits, periods, underscores and hyphens. Empty components, trailing periods, Windows device names, and traversal are rejected. Paths are limited to 1,024 bytes and components to 255 bytes. Use consistent case because Windows loose files and ZIP entries can differ in case handling.
- Native paths are trusted configuration and use `std::filesystem::path`. The engine converts them to UTF-8 for PhysicsFS. The CLI inherits the Windows narrow argument encoding. The native API accepts Unicode paths directly.
- The owner configures mounts and save storage before handing access to an I/O consumer. Keep the session alive until all reads and writes finish. Callers coordinate operations on the same save file. PhysicsFS symbolic links stay disabled. Keep configured native roots stable while mounted.
- Reads are synchronous and allocate up to a caller-supplied limit, 64 MiB by default. Route blocking calls through the Jobs I/O lane from interactive subsystems. Save writes overwrite in place and report short writes and close failures. A failed write may leave a partial save.

Incremental Debug and Release builds each passed all 16 CTest cases. Tests used a temporary save directory with a Unicode native name, verified persistence across sessions, and exercised the headless CLI with loose and deflated ZIP sources. SDK provenance verification passed for all 25 pinned packages, their submodules, and three local generator packages.

## Section 5 – Usage Examples

```powershell
.\build\debug\bin\WiseEngine.exe --fs-read 'F:\Content\World.zip' 'terrain/height.bin'
```

Pass a loose directory in place of the archive to read the same logical resource. From C++, construct `CE::AssetManager::FileSystem`, call `mountContent(source)`, and check `readContent("terrain/height.bin")` for either a byte vector or a `FileError`. Call `configureUserSaves()` before using `readSave` or `writeSave`.
