# Third-party notices

Downloaded SDK source and SDK binaries are not vendored in the public engine repository. Exact upstream repositories, releases and archive hashes are listed in `thirdparty/dependencies.lock.json`; the SDK inventory is in `docs/THIRD_PARTY.md`.

Every downloaded SDK retains its upstream license, notices, and nested dependency files. Diligent contains additional third-party components under its recursive submodules, and Boost contains component-specific notices. Inspect the actual pinned source trees and carry their required notices when distributing compiled libraries, tools, source snapshots or recovery ZIPs.

The engine's integration code does not relicense upstream SDK code. No engine-source license choice is made by this milestone.
