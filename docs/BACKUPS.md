# Milestone backups and restore

## Required milestone sequence

1. Complete the milestone acceptance gate and update its validation record.
2. Review/stage project-owned files; keep downloaded SDKs, credentials, logs and build products out of the public repository.
3. Commit the milestone and create its `v<version>` tag.
4. Run `python cmake/BackupMilestone.py --version <version>`. It requires a clean tracked source state and refuses to overwrite a backup.
5. Only after ZIP verification succeeds, push the source branch and milestone tag to the authorized repository.

This procedure applies after every completed milestone. It does not automatically advance to another milestone or start a scheduled job.

## What the ZIP preserves

`backups/WiseEngine-<version>-<UTC timestamp>.zip` contains engine source, build scripts, documentation, lockfile, SDK working trees and their Git metadata, original cached SDK archives and generator wheels. The embedded manifest records the source commit and SHA-256 of every file; a `.zip.sha256` sidecar verifies the complete archive.

Build trees, previous ZIPs, the engine Git database, IDE/task settings, Python bytecode caches, and `thirdparty/python-runtime` are excluded. SDK Git metadata remains so offline provenance verification works after recovery. Build outputs and the Python environment are regenerated. Backups remain local and are not committed or published to GitHub.

## Restore safely

Choose a new empty directory, keeping the active engine and ZIP intact. Verify first:

```powershell
python cmake/BackupMilestone.py --verify backups/WiseEngine-0.0.1-<timestamp>.zip
Get-FileHash backups/WiseEngine-0.0.1-<timestamp>.zip -Algorithm SHA256
```

Compare the archive hash against its sidecar, then extract using Windows Explorer or a ZIP64-capable tool. Open PowerShell in the restored root:

```powershell
python cmake/FetchThirdParty.py --offline
python cmake/VerifyThirdParty.py
.\build.ps1 -Config Debug -Target all -Tests
```

The compiler/Windows SDK/Python/Git must be installed independently. The offline command reconstructs only the local generator environment from cached wheels. It does not install global Python packages or restore build outputs. To recover engine Git history, clone the public repository at the manifest's `sourceCommit`; the ZIP is a source/SDK recovery snapshot rather than a Git history backup.
