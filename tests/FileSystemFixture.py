#!/usr/bin/env python3
"""Run from the engine root: python tests/FileSystemFixture.py <test-exe> <engine-exe>."""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("test_exe", type=Path)
    parser.add_argument("engine_exe", type=Path)
    args = parser.parse_args()

    # Fixture setup. Generate archive data independently of the runtime reader.
    try:
        for executable in (args.test_exe, args.engine_exe):
            if not executable.is_file():
                raise FileNotFoundError(executable)
        with tempfile.TemporaryDirectory(prefix="wiseengine-fs-") as temporary:
            root = Path(temporary)
            loose = root / "loose"
            (loose / "terrain").mkdir(parents=True)
            (loose / "terrain" / "height.bin").write_bytes(b"height")
            (loose / "empty.bin").touch()
            (loose / "priority.txt").write_bytes(b"loose")
            (loose / "loose-only.txt").write_bytes(b"loose")
            archive = root / "content.zip"
            with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as package:
                package.writestr("terrain/height.bin", b"height")
                package.writestr("priority.txt", b"zip")
            (root / "broken.zip").write_bytes(b"invalid archive")

            # Execution. Check the native tool path without a video driver.
            subprocess.run([args.test_exe.resolve(), root], check=True, timeout=10)
            environment = dict(os.environ, SDL_VIDEO_DRIVER="wiseengine-invalid-driver")
            for source in (loose, archive):
                result = subprocess.run(
                    [args.engine_exe.resolve(), "--fs-read", source, "terrain/height.bin"],
                    capture_output=True, text=True, check=True, timeout=10, env=environment,
                )
                if result.stdout.strip() != "bytes=6":
                    raise ValueError(f"Unexpected filesystem probe output: {result.stdout!r}")
            missing = subprocess.run(
                [args.engine_exe.resolve(), "--fs-read", archive, "missing.bin"],
                capture_output=True, text=True, timeout=10, env=environment,
            )
            if missing.returncode != 1 or "Open for reading" not in missing.stderr:
                raise ValueError(f"Unexpected missing-file result: {missing.returncode}, {missing.stderr!r}")
    except (OSError, subprocess.SubprocessError, ValueError) as error:
        print(f"File-system fixture failed: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError):
            print(error.stderr or error.stdout or "", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
