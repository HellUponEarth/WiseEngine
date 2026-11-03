# Section 1 – C++ Code

App owns the SDL video reference and the resizable window on the main thread. Startup logs through spdlog. Escape, a window close request, and SDL quit end the loop. The window is destroyed before the video reference is released, including when startup fails after acquiring video.

Input keeps keyboard and mouse state for each event batch. Held state survives between batches. Press and release edges can both be set for a quick tap. Mouse motion and wheel deltas accumulate until the next batch. Focus loss releases held controls. Read the snapshot after a successful pump and before the next pump.

# Section 2 – Build Configuration

App links Core, Input and spdlog. Input links SDL3 for its event types. The shell records logical and pixel dimensions separately. It waits for the first event and processes up to 256 events per batch. Diagnostic title updates happen at most once per batch. Use `--quiet-title` to skip title formatting and updates.

# Section 3 – Scripts

Use `build.ps1` for Debug and Release. After validation and tagging, run `python cmake/BackupMilestone.py --version 0.0.2` to make and verify a recovery ZIP. The backup and its checksum are stored under `backups`.

# Section 4 – Documentation

On 3 November 2026, incremental builds using the validated Windows x64 build trees passed 10 of 10 CTest cases in both Debug and Release.

- The SDL dummy-driver test covered invalid-driver startup and retry, same-batch key edges, repeat, mouse buttons and wheel, focus loss, foreign-window filtering, resize, Escape, close and quit. It checked explicit and destructor cleanup.
- The Release executable returned 1 and logged the SDL error when given an invalid video driver. Its version query still returned 0 in that environment.
- The live Debug window displayed keyboard and pointer activity. Maximize changed its reported size from 960×540 to 1920×1009 and restore returned it to 960×540. Escape exited with code 0. The live Release window kept a static title with `--quiet-title` and exited with code 0 after Alt+F4. Mouse button transitions were verified through the SDL test queue.

The client area is the native SDL shell. GPU renderer bring-up is scheduled for 0.0.8.

# Section 5 – Usage Examples

```powershell
.\build.ps1 -Config Debug -Target all -Tests
.\build\debug\bin\WiseEngine.exe
.\build\debug\bin\WiseEngine.exe --quiet-title
```

Press a key or move the pointer to see input activity in the title. Resize the window to check its reported dimensions. Press Escape to exit.
