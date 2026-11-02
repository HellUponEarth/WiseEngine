#!/usr/bin/env python3
"""Check installed SDK pins and write a local provenance report without networking.

How to run:
    python cmake/VerifyThirdParty.py

Requires Git, the dependency lock file, and completed SDK downloads.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

VENDOR = Path(__file__).resolve().parents[1] / "thirdparty"
GIT = ["git", "-c", f"safe.directory={VENDOR.as_posix()}/*"]


def git(path, *args):
    return subprocess.check_output([*GIT, "-C", str(path), *args], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    if not shutil.which("git"):
        parser.error("Git is required in PATH")
    root = Path(__file__).resolve().parents[1]
    vendor = root / "thirdparty"
    manifest = json.loads((vendor / "dependencies.lock.json").read_text(encoding="utf-8"))
    installed = []
    for entry in manifest["dependencies"]:
        folder = vendor / entry["name"]
        record = {"name": entry["name"], "version": entry["version"]}
        if entry["kind"] == "git":
            actual = git(folder, "rev-parse", "HEAD")
            if actual != entry["commit"]:
                raise RuntimeError(f"Revision mismatch: {entry['name']}")
            if git(folder, "remote", "get-url", "origin") != entry["url"]:
                raise RuntimeError(f"Origin mismatch: {entry['name']}")
            subprocess.run([*GIT, "-C", str(folder), "diff", "--exit-code", "HEAD", "--ignore-submodules=untracked"],
                           check=True, stdout=subprocess.DEVNULL)
            record["commit"] = actual
            if entry.get("submodules"):
                status = git(folder, "submodule", "status", "--recursive", "--", *entry["submodules"])
                if any(line.startswith(("-", "+", "U")) for line in status.splitlines()):
                    raise RuntimeError(f"Missing or modified pinned submodule: {entry['name']}")
                for line in status.splitlines():
                    subpath = line.strip().split()[1]
                    subprocess.run([*GIT, "-C", str(folder / subpath), "diff", "HEAD", "--exit-code", "--ignore-submodules=untracked"], check=True, stdout=subprocess.DEVNULL)
                record["submodules"] = status.splitlines()
        else:
            marker = json.loads((folder / ".customengine-sdk.json").read_text(encoding="utf-8"))
            if marker != entry:
                raise RuntimeError(f"Archive metadata mismatch: {entry['name']}")
            with (vendor / "_downloads" / entry["filename"]).open("rb") as stream:
                digest = hashlib.file_digest(stream, "sha256").hexdigest()
            if digest != entry["sha256"]:
                raise RuntimeError(f"Archive checksum mismatch: {entry['name']}")
            record["sha256"] = digest
        installed.append(record)
    interpreter = vendor / "python-runtime/Scripts/python.exe"
    query = "import importlib.metadata as m,json; print(json.dumps({n:m.version(n) for n in ['libclang','Jinja2','MarkupSafe']}))"
    packages = json.loads(subprocess.check_output([str(interpreter), "-c", query], text=True))
    expected = dict(item.split("==", 1) for item in manifest["pythonPackages"])
    if packages != expected:
        raise RuntimeError("Project-local generator package versions do not match the manifest")
    wheel_hashes = json.loads((vendor / "_wheels/sha256.json").read_text())
    for name, expected_hash in wheel_hashes.items():
        if hashlib.sha256((vendor / "_wheels" / name).read_bytes()).hexdigest() != expected_hash:
            raise RuntimeError(f"Generator wheel checksum mismatch: {name}")
    report = {"dependencies": installed, "pythonPackages": packages}
    (vendor / "dependencies.installed.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {len(installed)} SDK packages, pinned submodules, and {len(packages)} local generator packages.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"SDK verification failed: {error}", file=sys.stderr)
        sys.exit(1)
