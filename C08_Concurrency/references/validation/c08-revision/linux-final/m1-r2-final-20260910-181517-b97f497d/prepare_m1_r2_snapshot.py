from __future__ import annotations

import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path

RUN_ID = "m1-r2-final-20260910-181517-b97f497d"
REPO = Path.cwd().resolve()
DISTRO = "LearnCPP-C08-Ubuntu-24.04"
SNAPSHOT = Path(rf"\\wsl.localhost\{DISTRO}\root\learncpp-c08-src\{RUN_ID}")
EVIDENCE = REPO / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / RUN_ID
EXCLUDE_DIRS = {".git", ".omx", ".codex", ".agents", ".omc", ".claude", ".vs", "_deps", "CMakeFiles", ".cache", "__pycache__"}
EXCLUDE_EXTS = {".exe", ".dll", ".pdb", ".ilk", ".obj", ".o", ".lib", ".a", ".so", ".dylib", ".zip", ".7z", ".tar", ".gz", ".xz", ".bz2", ".bin", ".tmp", ".log"}
EXCLUDE_NAMES = {"CMakeCache.txt", "compile_commands.json"}
CXX_EXTS = {".cpp", ".hpp", ".h", ".cmake"}


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def walk(root: Path):
    stack = [root]
    while stack:
        cur = stack.pop()
        for entry in cur.iterdir():
            if entry.is_dir():
                if entry.name.startswith(".") or entry.name in EXCLUDE_DIRS or entry.name.lower().startswith("build"):
                    continue
                stack.append(entry)
            elif entry.is_file():
                rel_parts = entry.relative_to(REPO).parts[:-1]
                if any(part.startswith(".") or part in EXCLUDE_DIRS or part.lower().startswith("build") for part in rel_parts):
                    continue
                if entry.name in EXCLUDE_NAMES or entry.suffix.lower() in EXCLUDE_EXTS:
                    continue
                yield entry


def copy_file(src: Path, copied: list[str], rows: list[str]) -> None:
    rel = src.relative_to(REPO)
    dst = SNAPSHOT / rel
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    text = rel.as_posix()
    copied.append(text)
    rows.append(f"{sha(src)}  {text}")


def main() -> int:
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    SNAPSHOT.mkdir(parents=True, exist_ok=True)
    copied: list[str] = []
    rows: list[str] = []
    for src in walk(REPO / "C08_Concurrency"):
        copy_file(src, copied, rows)
    for src in walk(REPO):
        if "C08_Concurrency" in src.relative_to(REPO).parts:
            continue
        if src.suffix.lower() == ".md":
            copy_file(src, copied, rows)
    copied.sort()
    rows.sort()
    (EVIDENCE / "copied-files.txt").write_text("\n".join(copied) + "\n", encoding="utf-8")
    (EVIDENCE / "source-manifest.sha256.txt").write_text("\n".join(rows) + "\n", encoding="utf-8")
    manifest_hash = hashlib.sha256(("\n".join(rows) + "\n").encode("utf-8")).hexdigest()

    old_manifest = REPO / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / "final-20260910-173010-d5356576" / "source-manifest.sha256.txt"
    old = {}
    if old_manifest.exists():
        for line in old_manifest.read_text(encoding="utf-8", errors="ignore").splitlines():
            if "  " in line:
                digest, rel = line.split("  ", 1)
                if rel.startswith("C08_Concurrency/exercises/") and (Path(rel).suffix.lower() in CXX_EXTS or Path(rel).name in {"CMakeLists.txt", "CMakePresets.json"}):
                    old[rel] = digest
    current = {}
    for line in rows:
        digest, rel = line.split("  ", 1)
        if rel.startswith("C08_Concurrency/exercises/") and (Path(rel).suffix.lower() in CXX_EXTS or Path(rel).name in {"CMakeLists.txt", "CMakePresets.json"}):
            current[rel] = digest
    cxx_changes = sorted(rel for rel, digest in current.items() if old.get(rel) != digest)
    cxx_deleted = sorted(set(old) - set(current))

    summary = {
        "runId": RUN_ID,
        "snapshot": str(SNAPSHOT),
        "copiedFileCount": len(copied),
        "manifestSha256": manifest_hash,
        "m1SolutionSha256": sha(REPO / "C08_Concurrency" / "exercises" / "M1_work_stealing_pool" / "solution.cpp"),
        "verifyStudentsSha256": sha(REPO / "C08_Concurrency" / "exercises" / "tools" / "verify_students.py"),
        "cxxChangesComparedToPreviousFinal": cxx_changes,
        "cxxDeletedComparedToPreviousFinal": cxx_deleted,
        "timestamp": datetime.now(timezone.utc).astimezone().isoformat(),
    }
    (EVIDENCE / "snapshot-summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
