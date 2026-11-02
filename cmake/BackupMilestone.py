#!/usr/bin/env python3
"""Create and verify a self-contained milestone recovery ZIP.

How to run from the engine root:
    python cmake/BackupMilestone.py --version 0.0.1
    python cmake/BackupMilestone.py --verify backups/WiseEngine-0.0.1-<timestamp>.zip

Requires Python 3.12+. SDKs and their local Git metadata/cached archives are included.
Build trees, the engine Git database, previous backups and the relocatable Python venv
are excluded. Restore into an empty directory and run FetchThirdParty.py --offline.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import sys
import zipfile

MANIFEST = 'WISEENGINE_BACKUP_MANIFEST.json'
ROOT_EXCLUDES = {'build', 'backups', '.git', '.vs', '.vscode', '.codex', '.agents'}
STORED_SUFFIXES = {'.zip', '.gz', '.xz', '.whl', '.pack', '.png', '.jpg', '.7z'}


def source_files(root):
    # Prune rebuildable trees before walking so a large build cannot dominate backup time.
    pending = [root]
    while pending:
        directory = pending.pop()
        for path in sorted(directory.iterdir()):
            relative = path.relative_to(root)
            if relative.parts[0] in ROOT_EXCLUDES or '__pycache__' in relative.parts:
                continue
            if relative.parts[:2] == ('thirdparty', 'python-runtime'):
                continue
            if path.is_symlink() or path.is_junction():
                raise RuntimeError(f'Backup refuses links outside its explicit file inventory: {relative}')
            if path.is_dir():
                pending.append(path)
            elif path.is_file():
                if relative.as_posix() == MANIFEST:
                    raise RuntimeError(f'Reserved backup manifest path already exists: {path}')
                yield path


def safe_name(name):
    path = PurePosixPath(name)
    if not name or path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name:
        raise ValueError(f'Unsafe backup path: {name}')
    return name


def create_backup(root, destination, version, commit):
    root = root.resolve()
    destination.parent.mkdir(parents=True, exist_ok=True)
    records = []
    with destination.open('xb') as output, zipfile.ZipFile(output, 'w', allowZip64=True, compression=zipfile.ZIP_DEFLATED, compresslevel=1) as package:
        for number, path in enumerate(source_files(root), 1):
            name = safe_name(path.relative_to(root).as_posix())
            before = path.stat()
            info = zipfile.ZipInfo.from_file(path, name)
            info.compress_type = zipfile.ZIP_STORED if path.suffix.lower() in STORED_SUFFIXES else zipfile.ZIP_DEFLATED
            digest = hashlib.sha256()
            size = 0
            with path.open('rb') as source, package.open(info, 'w', force_zip64=True) as target:
                while block := source.read(1024 * 1024):
                    digest.update(block)
                    size += len(block)
                    target.write(block)
            after = path.stat()
            if before.st_size != after.st_size or before.st_mtime_ns != after.st_mtime_ns or size != before.st_size:
                raise RuntimeError(f'Source changed during backup: {path}')
            records.append({'path': name, 'size': size, 'sha256': digest.hexdigest()})
            if number % 25000 == 0:
                print(f'Archived {number} files...', flush=True)
        manifest = {'schema': 1, 'engine': 'WiseEngine', 'version': version, 'sourceCommit': commit,
                    'createdUtc': datetime.now(timezone.utc).isoformat(), 'files': records,
                    'restore': 'Extract into an empty directory; python cmake/FetchThirdParty.py --offline; python cmake/VerifyThirdParty.py; .\\build.ps1 -Config Debug -Target all -Tests'}
        package.writestr(MANIFEST, json.dumps(manifest, indent=2) + '\n')
    return verify_backup(destination)


def verify_backup(path):
    # Every payload byte is checked, not merely the ZIP directory or a sample of files.
    with zipfile.ZipFile(path) as package:
        infos = package.infolist()
        names = [safe_name(item.filename) for item in infos]
        if len(set(names)) != len(names):
            raise ValueError('Duplicate ZIP members')
        if any(stat.S_ISLNK(item.external_attr >> 16) for item in infos):
            raise ValueError('ZIP links are not supported')
        manifest = json.loads(package.read(MANIFEST))
        if manifest.get('schema') != 1 or manifest.get('engine') != 'WiseEngine':
            raise ValueError('Unsupported backup manifest')
        records = manifest['files']
        expected = {safe_name(item['path']) for item in records}
        if len(expected) != len(records) or set(names) != expected | {MANIFEST}:
            raise ValueError('ZIP inventory does not match the manifest')
        for number, item in enumerate(records, 1):
            digest = hashlib.sha256()
            count = 0
            with package.open(item['path']) as stream:
                while block := stream.read(1024 * 1024):
                    digest.update(block)
                    count += len(block)
            if count != item['size'] or digest.hexdigest() != item['sha256']:
                raise ValueError(f'Backup content mismatch: {item["path"]}')
            if number % 25000 == 0:
                print(f'Verified {number} files...', flush=True)
        return {'version': manifest['version'], 'sourceCommit': manifest['sourceCommit'],
                'files': len(records), 'uncompressedBytes': sum(item['size'] for item in records)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', help='Completed milestone version, for example 0.0.1')
    parser.add_argument('--verify', type=Path, help='Verify an existing recovery ZIP instead of creating one')
    args = parser.parse_args()
    if args.verify:
        print(json.dumps(verify_backup(args.verify), indent=2))
        return 0
    if not args.version or not re.fullmatch(r'\d+\.\d+\.\d+', args.version):
        parser.error('--version MAJOR.MINOR.PATCH is required when creating a backup')
    root = Path(__file__).resolve().parents[1]
    version_source = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    if f'VERSION {args.version} ' not in version_source:
        parser.error('Requested version does not match CMakeLists.txt')
    commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    dirty = subprocess.check_output(['git', '-C', str(root), 'status', '--porcelain'], text=True).strip()
    if dirty:
        raise RuntimeError('Commit source changes before creating a milestone backup')
    stamp = datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S-%fZ')
    final = root / 'backups' / f'WiseEngine-{args.version}-{stamp}.zip'
    partial = final.with_suffix('.zip.partial')
    report = create_backup(root, partial, args.version, commit)
    if final.exists():
        raise FileExistsError(final)
    partial.rename(final)
    with final.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    final.with_suffix('.zip.sha256').write_text(f'{digest}  {final.name}\n', encoding='ascii')
    print(json.dumps({'backup': str(final), 'sha256': digest, **report}, indent=2))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, RuntimeError, zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        print(f'Milestone backup failed: {error}', file=sys.stderr)
        sys.exit(1)
