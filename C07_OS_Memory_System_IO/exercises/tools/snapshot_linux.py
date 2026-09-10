"""Create an ext4 Linux snapshot for C07 validation without copying build output."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable

sys.dont_write_bytecode = True
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

DEFAULT_SOURCE = Path("/mnt/f/CPPTrain/LearnCPP")
TASK_ROOT = Path("/root/learncpp-c07")
DEFAULT_SNAPSHOT = TASK_ROOT / "snapshots"
FSMOUNT_TIMEOUT_SECONDS = 3
COURSE = "C07_OS_Memory_System_IO"
PUBLIC_INPUTS = [
    Path(COURSE),
    Path("C01_Build_Compile_Link/exercises/include"),
    Path("C01_Build_Compile_Link/exercises/tools/process_runner.py"),
    Path("C02_Objects_Lifetime_Ownership/exercises/tools/audit_student.py"),
    Path("C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py"),
]
EXCLUDED_DIRS = {
    ".git", ".omx", "build", ".vs", ".cache", "__pycache__", "_deps",
    "CMakeFiles", "Testing", "references/validation",
}
EXCLUDED_SUFFIXES = {
    ".exe", ".dll", ".lib", ".a", ".so", ".dylib", ".o", ".obj", ".pdb",
    ".ilk", ".idb", ".ninja_log", ".ninja_deps", ".tlog",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def fs_type(path: Path) -> str:
    try:
        return subprocess.run(["findmnt", "-n", "-o", "FSTYPE", "-T", str(path)], check=True,
                              capture_output=True, text=True,
                              timeout=FSMOUNT_TIMEOUT_SECONDS).stdout.strip()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return "unknown"


def acceptable_ext4(name: str) -> bool:
    return name == "ext4"


def excluded(rel: Path) -> bool:
    posix = rel.as_posix()
    if rel.suffix.lower() in EXCLUDED_SUFFIXES:
        return True
    if "references/validation" in posix:
        return True
    return any(part in EXCLUDED_DIRS or "/".join(rel.parts[:i + 1]) in EXCLUDED_DIRS
               for i, part in enumerate(rel.parts)) or posix.endswith("CMakeCache.txt")


def iter_files(source_root: Path, roots: Iterable[Path]) -> Iterable[Path]:
    for item in roots:
        src = source_root / item
        if not src.exists():
            raise FileNotFoundError(src)
        if src.is_symlink():
            continue
        if src.is_file():
            if not excluded(item):
                yield item
            continue
        for current, dirs, files in os.walk(src):
            current_path = Path(current)
            rel_dir = current_path.relative_to(source_root)
            kept_dirs = []
            for name in dirs:
                child_rel = rel_dir / name
                if not (source_root / child_rel).is_symlink() and not excluded(child_rel):
                    kept_dirs.append(name)
            dirs[:] = kept_dirs
            for name in files:
                child = current_path / name
                rel = child.relative_to(source_root)
                if not child.is_symlink() and not excluded(rel):
                    yield rel


def make_snapshot(source: Path, snapshot: Path, require_linux: bool = True,
                  require_taskroot_ext4: bool = True) -> dict:
    if require_linux and platform.system() != "Linux":
        raise RuntimeError("snapshot_linux must run inside WSL/Linux")
    source = source.resolve()
    snapshot = snapshot.resolve()
    if require_taskroot_ext4:
        if not is_relative_to(snapshot, TASK_ROOT.resolve()) or "snapshots" not in PurePosixPath(snapshot.as_posix()).parts:
            raise RuntimeError("snapshot path must be a new /root/learncpp-c07/.../snapshots/<runid> directory")
        probe = snapshot.parent
        while not probe.exists() and probe != probe.parent:
            probe = probe.parent
        fs = fs_type(probe)
        if not acceptable_ext4(fs):
            raise RuntimeError(f"snapshot destination is not ext4-backed: {fs}")
    else:
        fs = fs_type(snapshot.parent) if snapshot.parent.exists() else "unchecked"
    if snapshot.exists():
        raise FileExistsError(f"snapshot already exists: {snapshot}")
    if not (source / COURSE).is_dir():
        raise FileNotFoundError(source / COURSE)
    snapshot.mkdir(parents=True)
    files = []
    for rel in sorted(set(iter_files(source, PUBLIC_INPUTS)), key=lambda p: p.as_posix()):
        src = source / rel
        dst = snapshot / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst, follow_symlinks=False)
        files.append({
            "path": rel.as_posix(),
            "source": str(src),
            "source_sha256": sha256(src),
            "snapshot_sha256": sha256(dst),
            "bytes": dst.stat().st_size,
        })
    manifest = {
        "verdict": "PASS",
        "source_root": str(source),
        "snapshot_root": str(snapshot),
        "destination_filesystem": fs,
        "ext4_verified": acceptable_ext4(fs) if require_taskroot_ext4 else False,
        "scope": "C07 plus C01/C02 public include/tools only; no C08, build output, symlinks, dependencies, or raw validation trees copied",
        "files": files,
    }
    (snapshot / "snapshot-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                                                     encoding="utf-8")
    return manifest


def self_check() -> int:
    with tempfile.TemporaryDirectory(prefix="c07-snapshot-src-") as src_dir, tempfile.TemporaryDirectory(prefix="c07-snapshot-dst-") as dst_dir:
        assert acceptable_ext4("ext4")
        assert not acceptable_ext4("ext2/ext3")
        assert not acceptable_ext4("9p")
        assert not acceptable_ext4("unknown")
        src = Path(src_dir)
        for rel in PUBLIC_INPUTS:
            path = src / rel
            if rel.suffix:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f"{rel.as_posix()}\n", encoding="utf-8")
            else:
                path.mkdir(parents=True, exist_ok=True)
        (src / COURSE / "README.md").write_text("# C07\n", encoding="utf-8")
        (src / COURSE / "build").mkdir()
        (src / COURSE / "build" / "skip.obj").write_bytes(b"obj")
        target = Path(dst_dir) / "snapshots" / "run1"
        result = make_snapshot(src, target, require_linux=False, require_taskroot_ext4=False)
        copied = {item["path"] for item in result["files"]}
        assert f"{COURSE}/README.md" in copied
        assert f"{COURSE}/build/skip.obj" not in copied
        assert not (target / "C08_Concurrency").exists()
        try:
            make_snapshot(src, target, require_linux=False, require_taskroot_ext4=False)
        except FileExistsError:
            pass
        else:
            raise AssertionError("existing snapshot was overwritten")
    print("snapshot_linux self-check PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--snapshot", type=Path, help="New /root/learncpp-c07/.../snapshots/<runid> directory")
    parser.add_argument("--dest-root", type=Path, default=DEFAULT_SNAPSHOT)
    parser.add_argument("--run-id")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    snapshot = args.snapshot
    if snapshot is None:
        if not args.run_id:
            parser.error("--snapshot or --run-id is required")
        snapshot = args.dest_root / args.run_id
    try:
        result = make_snapshot(args.source, snapshot)
    except Exception as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"PASS: {result['snapshot_root']} ({len(result['files'])} files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
