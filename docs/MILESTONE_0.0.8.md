# WiseEngine 0.0.8

## Section 1 – C++ Code

`src/Renderer/Dx12Scene.cpp` owns one Diligent DX12 device, immediate context, RGBA8 swap chain and D32 depth buffer. It uploads an indexed cube and a generated checkerboard once, updates a GLM perspective camera matrix, and renders one indexed draw per frame. App owns the SDL window, passes its Win32 handle to Renderer, and tears down Renderer before destroying the window.

Rendering follows the SDL drawable pixel size. Resize recreates the swap-chain views and updates the viewport and camera aspect. The app waits for events while minimized and resumes drawing when the window is restored.

## Section 2 – Build Configuration

CMake compiles `src/Renderer/Shaders/BringUp.hlsl` with the pinned DXC into Shader Model 6.0 vertex and pixel DXIL. The bytecode is embedded in the executable. The staged DXC runtime supports Diligent's bytecode reflection. The runtime renderer uses DX12 and requires a compatible Windows adapter.

## Section 3 – Scripts

Use `python cmake/VerifyThirdParty.py` and the normal build script. To register the GPU smoke in CTest, run from the engine root:

```powershell
.\build.ps1 -Config Debug -Target all -Tests -CMakeArgs '-DWISEENGINE_TEST_DX12=ON'
.\build.ps1 -Config Release -Target all -Tests -CMakeArgs '-DWISEENGINE_TEST_DX12=ON'
```

Use `python cmake/BackupMilestone.py --version 0.0.8` after committing and tagging to create and verify the milestone ZIP.

## Section 4 – Documentation

The normal executable opens a VSync-paced rotating checkerboard cube with the D3D12 debug layer enabled. Validation requires Windows Graphics Tools. The scene uses the application thread's immediate graphics context. Render-thread ownership is a later milestone.

- `--no-render` opens the event-driven SDL shell without a GPU device.
- `--no-gpu-validation` runs the scene without the debug layer and its CPU overhead.
- `--dx12-smoke` uses a hidden native window, renders three frames, and verifies that readback contains both checker colors and the background. It checks the native debug message queue for errors or corruption at teardown. The staging readback adds a GPU idle wait only in smoke mode. Normal shutdown also waits for outstanding GPU work.

Debug and Release incremental builds each passed all 18 CTest cases with the opt-in DX12 smoke enabled. GPU readback verified both checker colors and the background on the NVIDIA GeForce RTX 4060 Ti. The live Debug window rendered at 960x540 and 1920x1009, suspended on minimize, resumed on restore, and exited through Escape without DX12 error or corruption messages. SDK provenance verification passed for all 25 packages, pinned submodules, and three local generator packages.

## Section 5 – Usage Examples

```powershell
.\build\debug\bin\WiseEngine.exe
.\build\debug\bin\WiseEngine.exe --dx12-smoke
```

Resize or maximize the window, minimize it, and restore it. The cube should retain its proportions and resume rotation. Press Escape to shut down.
