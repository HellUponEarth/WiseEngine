#!/usr/bin/env python3
"""Download the pinned SDKs without replacing existing local changes.

How to run:
    python cmake/FetchThirdParty.py
    python cmake/FetchThirdParty.py --jobs 4

Git and network access are required. The lock file records upstream commits
and SHA-256 hashes; CMake itself never downloads project dependencies.
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
import venv


ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "thirdparty"
GIT = ["git", "-c", "core.symlinks=false", "-c", f"safe.directory={VENDOR.as_posix()}/*"]


def run_git(*args, cwd=None):
    result = subprocess.run(
        [*GIT, *args], cwd=cwd, text=True, capture_output=True,
        env={**os.environ, "GIT_TERMINAL_PROMPT": "0", "GIT_LFS_SKIP_SMUDGE": "1"},
    )
    if result.returncode:
        raise RuntimeError(f"git {' '.join(args)} failed:\n{result.stderr.strip()}")
    return result.stdout.strip()


def local_path(relative):
    path = (VENDOR / relative).resolve()
    if not path.is_relative_to(VENDOR.resolve()) or path == VENDOR.resolve():
        raise ValueError(f"Destination must remain inside thirdparty: {relative}")
    return path


OFFLINE = False


def fetch_repository(entry):
    destination = local_path(entry["name"])
    if destination.exists() and not (destination / ".git").is_dir():
        # Preserve the supplied scaffold rather than overwriting its placeholders.
        known = {"fmt", "spdlog", "yaml-cpp", "stb"}
        files = {p.name for p in destination.iterdir()}
        if entry["name"] not in known or not files.issubset({"CMakeLists.txt", "README.txt"}):
            raise RuntimeError(f"Refusing to replace an existing non-repository: {destination}")
        backup = local_path(f"_original_scaffold/{entry['name']}")
        if backup.exists():
            raise RuntimeError(f"Scaffold backup already exists: {backup}")
        backup.parent.mkdir(parents=True, exist_ok=True)
        destination.rename(backup)
    if OFFLINE and not (destination / ".git").is_dir():
        raise RuntimeError(f"SDK checkout missing in offline mode: {destination}")
    if not (destination / ".git").is_dir():
        destination.mkdir(parents=True, exist_ok=True)
        run_git("init", str(destination))
        run_git("config", "core.symlinks", "false", cwd=destination)
        run_git("remote", "add", "origin", entry["url"], cwd=destination)
    head = subprocess.run([*GIT, "rev-parse", "HEAD"], cwd=destination,
                          text=True, capture_output=True)
    if head.returncode == 0 and head.stdout.strip() != entry["commit"]:
        raise RuntimeError(f"Existing checkout has a different revision: {destination}")
    if head.returncode != 0 and OFFLINE:
        raise RuntimeError(f"SDK revision missing in offline mode: {destination}")
    if head.returncode != 0:
        run_git("fetch", "--depth", "1", "origin", entry["commit"], cwd=destination)
        run_git("-c", "advice.detachedHead=false", "checkout", "--detach", "FETCH_HEAD", cwd=destination)
    if entry.get("submodules") and not OFFLINE:
        run_git("submodule", "update", "--init", "--recursive", "--depth", "1",
                "--jobs", "4", "--", *entry["submodules"], cwd=destination)
    print(f"Ready {entry['name']}: {entry['version']} ({entry['commit'][:12]})", flush=True)


def fetch_archive(entry):
    destination = local_path(entry["name"])
    marker = destination / ".customengine-sdk.json"
    if marker.is_file():
        if json.loads(marker.read_text(encoding="utf-8")) != entry:
            raise RuntimeError(f"Archive pin differs from installed SDK: {destination}")
        print(f"Ready {entry['name']}: {entry['version']}", flush=True)
        return
    if destination.exists():
        raise RuntimeError(f"Refusing to overwrite incomplete or unknown SDK: {destination}")
    cache = local_path("_downloads")
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / entry["filename"]
    if OFFLINE and not archive.exists():
        raise RuntimeError(f"Archive cache missing in offline mode: {archive}")
    if not archive.exists():
        partial = archive.with_name(archive.name + ".partial")
        with urllib.request.urlopen(entry["url"], timeout=120) as response, partial.open("wb") as output:
            shutil.copyfileobj(response, output)
        partial.replace(archive)
    with archive.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest.lower() != entry["sha256"].lower():
        raise RuntimeError(f"SHA-256 mismatch for {archive}")
    destination.mkdir(parents=True)
    if entry["format"] == "zip":
        with zipfile.ZipFile(archive) as package:
            for member in package.infolist():
                if not (destination / member.filename).resolve().is_relative_to(destination):
                    raise RuntimeError("Unsafe path in ZIP archive")
            package.extractall(destination)
    else:
        with tarfile.open(archive) as package:
            package.extractall(destination, filter="data")
    marker.write_text(json.dumps(entry, indent=2) + "\n", encoding="utf-8")
    print(f"Ready {entry['name']}: {entry['version']}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, default=4, help="Concurrent SDK downloads (1-8).")
    parser.add_argument("--offline", action="store_true", help="Use installed SDKs and cached wheels without network access.")
    args = parser.parse_args()
    global OFFLINE
    OFFLINE = args.offline
    if not 1 <= args.jobs <= 8:
        parser.error("--jobs must be between 1 and 8")
    if not shutil.which("git"):
        parser.error("Git is required and must be available in PATH")
    lock = VENDOR / "dependencies.lock.json"
    if not lock.is_file():
        parser.error(f"Dependency lock file is missing: {lock}")
    manifest = json.loads(lock.read_text(encoding="utf-8"))
    failures = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        work = {pool.submit(fetch_repository if item["kind"] == "git" else fetch_archive, item): item
                for item in manifest["dependencies"]}
        for future in concurrent.futures.as_completed(work):
            try:
                future.result()
            except Exception as error:
                failures.append(f"{work[future]['name']}: {error}")
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    # Build Helpers: isolate upstream code generation from the user's Python packages.
    environment = local_path("python-runtime")
    interpreter = environment / "Scripts/python.exe"
    if not interpreter.exists():
        venv.EnvBuilder(with_pip=True).create(environment)
    wheels = local_path("_wheels")
    wheels.mkdir(exist_ok=True)
    packages = manifest.get("pythonPackages", [])
    if packages:
        if OFFLINE:
            hashes_file = wheels / "sha256.json"
            if not hashes_file.is_file():
                raise RuntimeError("Offline wheel cache hashes are missing")
            for name, expected in json.loads(hashes_file.read_text()).items():
                if hashlib.sha256((wheels / name).read_bytes()).hexdigest() != expected:
                    raise RuntimeError(f"Offline wheel hash mismatch: {name}")
        else:
            subprocess.run([str(interpreter), "-m", "pip", "--isolated", "--disable-pip-version-check",
                        "download", "--dest", str(wheels), *packages], check=True)
        subprocess.run([str(interpreter), "-m", "pip", "--isolated", "--disable-pip-version-check",
                        "install", "--no-index", "--find-links", str(wheels), *packages], check=True)
        wheel_hashes = {item.name: hashlib.sha256(item.read_bytes()).hexdigest()
                        for item in wheels.glob("*.whl")}
        (wheels / "sha256.json").write_text(json.dumps(wheel_hashes, indent=2) + "\n", encoding="utf-8")
    print("All pinned SDKs are available locally.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"SDK bootstrap failed: {error}", file=sys.stderr)
        sys.exit(1)
