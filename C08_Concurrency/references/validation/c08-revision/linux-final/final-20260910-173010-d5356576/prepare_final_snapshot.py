from __future__ import annotations

import hashlib
import json
import os
import shutil
from datetime import datetime, timezone
from pathlib import Path


RUN_ID = "final-20260910-173010-d5356576"
REPO_ROOT = Path.cwd().resolve()
DISTRO = "LearnCPP-C08-Ubuntu-24.04"
GUEST_SNAPSHOT_ROOT = Path(rf"\\wsl.localhost\{DISTRO}\root\learncpp-c08-src\{RUN_ID}")
EVIDENCE_ROOT = REPO_ROOT / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / RUN_ID

EXCLUDE_DIRS = {
    ".git",
    ".omx",
    ".codex",
    ".agents",
    ".omc",
    ".claude",
    ".vs",
    "_deps",
    "CMakeFiles",
    ".cache",
    ".pytest_cache",
    "__pycache__",
}
EXCLUDE_EXTS = {
    ".exe",
    ".dll",
    ".pdb",
    ".ilk",
    ".obj",
    ".o",
    ".lib",
    ".a",
    ".so",
    ".dylib",
    ".zip",
    ".7z",
    ".tar",
    ".gz",
    ".xz",
    ".bz2",
    ".bin",
    ".tmp",
    ".log",
}
EXCLUDE_NAMES = {"CMakeCache.txt", "compile_commands.json"}
CXX_EXTS = {".cpp", ".hpp", ".h", ".cmake"}


def iso_now() -> str:
    return datetime.now(timezone.utc).astimezone().isoformat()


def skip_dir(path: Path) -> bool:
    name = path.name
    if name in EXCLUDE_DIRS:
        return True
    if name.startswith("."):
        return True
    if name.lower().startswith("build"):
        return True
    return False


def walk_files(root: Path):
    stack = [root]
    while stack:
        current = stack.pop()
        try:
            entries = list(current.iterdir())
        except OSError:
            continue
        for entry in entries:
            if entry.is_dir():
                if not skip_dir(entry):
                    stack.append(entry)
            elif entry.is_file():
                yield entry


def copyable_file(path: Path) -> bool:
    if path.name in EXCLUDE_NAMES:
        return False
    if path.suffix.lower() in EXCLUDE_EXTS:
        return False
    if any(part.startswith(".") or part in EXCLUDE_DIRS or part.lower().startswith("build") for part in path.relative_to(REPO_ROOT).parts[:-1]):
        return False
    return True


def copy_file(src: Path, copied: list[str]) -> None:
    rel = src.relative_to(REPO_ROOT)
    dst = GUEST_SNAPSHOT_ROOT / rel
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    copied.append(rel.as_posix())


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    EVIDENCE_ROOT.mkdir(parents=True, exist_ok=True)
    GUEST_SNAPSHOT_ROOT.mkdir(parents=True, exist_ok=True)

    snapshot_start_dt = datetime.now()
    snapshot_start = iso_now()
    (EVIDENCE_ROOT / "snapshot-start.txt").write_text(snapshot_start + "\n", encoding="utf-8")

    copied: list[str] = []

    c08_root = REPO_ROOT / "C08_Concurrency"
    for path in walk_files(c08_root):
        if copyable_file(path):
            copy_file(path, copied)

    for path in walk_files(REPO_ROOT):
        if "C08_Concurrency" in path.relative_to(REPO_ROOT).parts:
            continue
        if path.suffix.lower() == ".md" and copyable_file(path):
            copy_file(path, copied)

    copied.sort()
    (EVIDENCE_ROOT / "copied-files.txt").write_text("\n".join(copied) + "\n", encoding="utf-8")

    manifest_rows: list[str] = []
    manifest_hash = hashlib.sha256()
    for rel in copied:
        src = REPO_ROOT / Path(rel)
        digest = sha256_file(src)
        row = f"{digest}  {rel}"
        manifest_rows.append(row)
        manifest_hash.update((row + "\n").encode("utf-8"))
    (EVIDENCE_ROOT / "source-manifest.sha256.txt").write_text("\n".join(manifest_rows) + "\n", encoding="utf-8")

    cpp_changed = []
    for path in walk_files(c08_root / "exercises"):
        if path.name == "CMakeLists.txt" or path.name == "CMakePresets.json" or path.suffix.lower() in CXX_EXTS:
            mtime = datetime.fromtimestamp(path.stat().st_mtime)
            if mtime > snapshot_start_dt:
                cpp_changed.append({
                    "path": path.relative_to(REPO_ROOT).as_posix(),
                    "lastWriteTime": mtime.astimezone().isoformat(),
                })

    snapshot_end = iso_now()
    (EVIDENCE_ROOT / "snapshot-end.txt").write_text(snapshot_end + "\n", encoding="utf-8")
    (EVIDENCE_ROOT / "cpp-files-modified-during-snapshot.json").write_text(
        json.dumps(cpp_changed, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    summary = {
        "runId": RUN_ID,
        "repoRoot": str(REPO_ROOT),
        "distro": DISTRO,
        "guestSnapshotRoot": str(GUEST_SNAPSHOT_ROOT),
        "copiedFileCount": len(copied),
        "manifestSha256": manifest_hash.hexdigest(),
        "snapshotStart": snapshot_start,
        "snapshotEnd": snapshot_end,
        "cppFilesModifiedDuringSnapshot": len(cpp_changed),
    }
    (EVIDENCE_ROOT / "snapshot-copy-summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
