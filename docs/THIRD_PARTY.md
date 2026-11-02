# SDK inventory and integration

## Selection and ownership

The 25 SDK packages below are copied or bootstrapped into `thirdparty` at exact Git revisions or archive SHA-256 pins. Diligent Core/Tools/FX includes required recursive submodules. Its ImGui implementation is reused; no duplicate ImGui library is linked. Optional ozz/Recast SDKs are downloaded but excluded from the default module implementation.

Core links only Math and fmt/spdlog. The standalone SDK smoke test links the selected SDK stack and exercises representative operations. SDK availability does not implement a window, renderer, streamed world, foliage or ocean backend. Standard C++20 provides atomics, fixed-width integers, ordinary containers and native filesystem operations; no separate Boost.Thread/Atomic binary is needed.

| Planned subsystem | SDKs | Integration responsibility |
|---|---|---|
| Core | fmt, spdlog | Baseline build logging; broader lifecycle follows later. |
| Platform/input | SDL3, Boost.Predef | Window/events, runtime CPU facts, build-time platform facts. |
| Math/identity | GLM, EnTT, selected Boost containers | Explicit types and generation-checked engine handles. |
| Threading/jobs | SDL3, enkiTS | Ownership, dependencies, bounded completion queues and safe shutdown. |
| Rendering | Diligent Core/Tools/FX, DXC, ImGui, Tracy | Diligent-backed DX12/Vulkan, materials, offline shaders and diagnostics. |
| Scripting | Luau VM/compiler/require, LuaBridge3 | One VM owner, checked handles, coarse native APIs and bounded execution. |
| Physics/audio | Jolt, miniaudio | World integration and fixed stepping; callback/lifetime safety. |
| Resources | PhysicsFS, Zstd, xxHash, JSON, YAML, stb | Mounted paths, versioned payloads and bounded residency. |
| Cooking | meshoptimizer, DirectXTex | Prepared LODs/textures and incremental output. |
| Network | standalone Asio | TCP/UDP transport; framing, cancellation and backpressure remain engine work. |
| Later optional systems | ozz-animation, Recast/Detour | Not initial 0.1 requirements unless scope changes. |

## Exact inputs

| SDK | Release label | Git commit / archive SHA-256 | Upstream |
|---|---|---|---|
| fmt | 11.1.4 | `123913715afeb8a437e6388b4473fcc4753e1c9a` | [source](https://github.com/fmtlib/fmt.git) |
| spdlog | v1.15.3 | `6fa36017cfd5731d617e1a934f0e5ea9c4445b13` | [source](https://github.com/gabime/spdlog.git) |
| yaml-cpp | 0.8.0 | `f7320141120f720aecc4c32be25586e7da9eb978` | [source](https://github.com/jbeder/yaml-cpp.git) |
| stb | HEAD | `d14d46f6b44456eb88beac58cba833a4135d4c89` | [source](https://github.com/nothings/stb.git) |
| SDL | release-3.4.18 | `829a65d769d935c4852f8159e964312c0957260a` | [source](https://github.com/libsdl-org/SDL.git) |
| glm | 1.0.3 | `8d1fd52e5ab5590e2c81768ace50c72bae28f2ed` | [source](https://github.com/g-truc/glm.git) |
| entt | v3.15.0 | `d4014c74dc3793aba95ae354d6e23a026c2796db` | [source](https://github.com/skypjack/entt.git) |
| enkiTS | v1.12 | `0289cf6ffce86697e1f46cb4eeca1c946bffec71` | [source](https://github.com/dougbinks/enkiTS.git) |
| JoltPhysics | v5.6.0 | `e77f175595e64cb44218cc9d9d56fc365ad0e36a` | [source](https://github.com/jrouwe/JoltPhysics.git) |
| luau | 0.745 | `2a3d8937c19c8fd9f7b72b6ec9068c62760e6588` | [source](https://github.com/luau-lang/luau.git) |
| LuaBridge3 | 3.0-rc13 | `64b3ec3d265578c742abd5d3939b133353952718` | [source](https://github.com/kunitoki/LuaBridge3.git) |
| tracy | v0.14.1 | `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c` | [source](https://github.com/wolfpld/tracy.git) |
| physfs | release-3.2.0 | `eb3383b532c5f74bfeb42ec306ba2cf80eed988c` | [source](https://github.com/icculus/physfs.git) |
| asio | asio-1-38-2 | `8806a6803cde7054c3049d3666d3ec36786568c5` | [source](https://github.com/chriskohlhoff/asio.git) |
| meshoptimizer | v1.3 | `9e1f07b159d3cb777f1c67ed31fc11fd117986f4` | [source](https://github.com/zeux/meshoptimizer.git) |
| DirectXTex | oct2025 | `232bbf2e39a37ea2e9b7b8331653fa69bd4797a2` | [source](https://github.com/microsoft/DirectXTex.git) |
| zstd | v1.5.7 | `f8745da6ff1ad1e7bab384bd1f9d742439278e99` | [source](https://github.com/facebook/zstd.git) |
| xxHash | v0.8.4 | `c87183a77d67f7d37e3d2d1b7eaac5e7c695e4f0` | [source](https://github.com/Cyan4973/xxHash.git) |
| json | v3.12.0 | `55f93686c01528224f448c19128836e7df245f72` | [source](https://github.com/nlohmann/json.git) |
| miniaudio | 0.11.26 | `877ea4a7c2601c7b0f2a44e83dfac42e6470fea6` | [source](https://github.com/mackron/miniaudio.git) |
| DiligentEngine | v2.5.6 | `5301bfd3bb36fe231fee31ae2a28a4e5175a7551` | [source](https://github.com/DiligentGraphics/DiligentEngine.git) |
| ozz-animation | 0.16.0 | `6cbdc790123aa4731d82e255df187b3a8a808256` | [source](https://github.com/guillaumeblanc/ozz-animation.git) |
| recastnavigation | v1.6.0 | `6dc1667f580357e8a2154c28b7867bea7e8ad3a7` | [source](https://github.com/recastnavigation/recastnavigation.git) |
| boost | 1.88.0 | `3621533e820dcab1e8012afd583c0c73cf0f77694952b81352bf38c1488f9cb4` | [source](https://archives.boost.io/release/1.88.0/source/boost_1_88_0.tar.gz) |
| dxc | v1.9.2609 | `ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1` | [source](https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/dxc_2026_09_29.zip) |

The stb label HEAD is informational; the recorded immutable commit is the actual input. Diligent submodule revisions are locked by the parent trees and verified recursively. SDK licenses and nested notices stay in the installed sources.

## Build and recovery

`python cmake/FetchThirdParty.py --jobs 4` performs the explicit network bootstrap. `--offline` uses installed SDKs/cached wheels when recovering from a full milestone ZIP. `python cmake/VerifyThirdParty.py` performs local provenance checks. No global Python package install is needed.

`thirdparty/CMakeLists.txt` is project-owned. SDK source folders are read-only. ABI settings use `/MDd` for Debug and `/MD` otherwise, one fmt implementation for spdlog, consistent JSON diagnostics, and optional Jolt double precision. Shader compilation uses the pinned DXC for DX12 DXIL and Vulkan SPIR-V. CryEngine's separate FXC/cache rules are not applied to this standalone Diligent project.

Explicit tool target: `cmake --build --preset debug --target WiseEngineAssetTools --parallel 6` from an initialized MSVC shell after configure. It exposes gltfpack, texassemble, texdiag and the Luau compiler/analyzer. DX11 is disabled, including DX11-only texconv.

Profiling and future SDK toggles are documented in `MILESTONE_0.0.1.md`. No external FFT/ocean SDK is required for the planned Diligent compute implementation; the complete ocean system remains engine work.

SDK Git checkouts use `core.symlinks=false` for portable Windows recovery ZIPs. Tracked symlinks are represented by their committed link-text blobs; Asio uses its actual `asio/include` directory rather than its compatibility link. This changes no upstream tracked content.
