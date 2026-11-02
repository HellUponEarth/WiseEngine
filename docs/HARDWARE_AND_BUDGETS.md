# Hardware and performance targets

## Build host validated for 0.0.1

Windows x64, Ryzen 7 5800X (16 logical processors), MSVC 19.38/VS2022, CMake 4.2.1, Ninja 1.12.0, Windows SDK 10.0.26100, Python 3.14. Toolchain details are recorded with the actual validation results. CMake minimum is 3.31; C++20 is required. Python 3.12+ is the bootstrap/recovery baseline.

## Reference PCs for 0.1

| Tier | CPU | GPU | RAM | Storage |
|---|---|---|---|---|
| Minimum | Ryzen 5 3600 | GTX 1060 6 GB | 16 GB | SATA SSD |
| Recommended | Ryzen 7 5800X | RTX 3070 | 32 GB | NVMe SSD |
| Target | Ryzen 9 7900X | RTX 4080 | 64 GB | NVMe Gen4 SSD |

## Provisional runtime budgets

Reference scene: configurable 16 x 16 km coastline; only budgeted cells/LODs remain resident. Proposed presentation target is 1920 x 1080 at 60 FPS, with 16.67 ms frame time. Initial planning envelopes are CPU work 6 ms, GPU work 8 ms and 2.67 ms headroom. CPU and GPU overlap; do not sum independent CPU/GPU measurements as an actual frame-time measurement.

Initial working-set target is approximately 752 MB across engine (128), Math/Core (16), Renderer (256), Physics (32), Audio (64) and resource cache (256). Define host RAM, committed memory and VRAM separately when real resource residency exists. This inherited planning allocation is not a measured 0.1 memory requirement.

Initial submission limits: 2,000 opaque and 500 transparent draws at minimum settings. Dense foliage must use batching/instancing and measured GPU/overdraw budgets; instance count alone is not a performance target. Alpha-tested foliage belongs in the masked path rather than ordinary blended sorting.

All values are planning targets. At 0.0.1 no renderer or game loop exists, so there is no frame-rate, VRAM, terrain traversal, foliage density or ocean-quality acceptance claim.
