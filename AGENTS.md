# WiseEngine Development Guidelines

The user-provided comment, scope, performance-toggle, script, and documentation rules apply.

- Engine root: this checkout. Preserve F:\CustomEngine as a separate project.
- CMake 3.31+, Ninja, Windows x64 MSVC, C++20, /W4 /WX /permissive-.
- Engine source belongs in src/<Module>; paired .h/.cpp for new classes, #pragma once, CE::<Module> namespaces.
- Math has no dependencies. Core depends only on Math and fmt/spdlog. Other engine-module edges are enforced in cmake/ModuleBoundaries.cmake.
- Core must not initialize Input, Renderer, SDL, physics, audio, or networking. Application orchestration belongs in App.
- Downloaded SDK source trees are read-only. Change adapters under thirdparty/integration instead.
- SDK pins live in thirdparty/dependencies.lock.json. No configure-time dependency fetches.
- 0.0.1 is a build baseline. SDL application behavior starts at 0.0.2; rendering starts at 0.0.8.
- After each completed milestone, run validation, commit/tag, create and verify a ZIP with cmake/BackupMilestone.py, then push to the authorized public repository.
- Backups must include source and installed SDKs; exclude rebuildable build trees and Python venvs. Never overwrite existing backups.
- Do not claim frame-rate, gameplay, rendering, or future milestone acceptance from a baseline build.
