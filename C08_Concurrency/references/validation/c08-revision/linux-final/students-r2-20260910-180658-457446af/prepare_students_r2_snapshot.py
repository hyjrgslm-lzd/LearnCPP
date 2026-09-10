from __future__ import annotations

import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path

RUN_ID = "students-r2-20260910-180658-457446af"
REPO = Path.cwd().resolve()
DISTRO = "LearnCPP-C08-Ubuntu-24.04"
SNAPSHOT = Path(rf"\\wsl.localhost\{DISTRO}\root\learncpp-c08-src\{RUN_ID}")
EVIDENCE = REPO / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / RUN_ID
SOURCE_ROOT = REPO / "C08_Concurrency" / "exercises"
EXCLUDE_DIRS = {".git", ".omx", ".codex", ".agents", ".omc", ".claude", ".vs", "_deps", "CMakeFiles", "__pycache__"}
EXCLUDE_EXTS = {".exe", ".dll", ".pdb", ".ilk", ".obj", ".o", ".lib", ".a", ".so", ".dylib", ".zip", ".7z", ".tar", ".gz", ".xz", ".bz2", ".bin", ".tmp", ".log"}
EXCLUDE_NAMES = {"CMakeCache.txt", "compile_commands.json"}


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


def main() -> int:
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    SNAPSHOT.mkdir(parents=True, exist_ok=True)
    copied: list[str] = []
    rows: list[str] = []
    for src in walk(SOURCE_ROOT):
        rel = src.relative_to(REPO)
        dst = SNAPSHOT / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        rel_text = rel.as_posix()
        copied.append(rel_text)
        rows.append(f"{sha(src)}  {rel_text}")
    copied.sort()
    rows.sort()
    manifest_hash = hashlib.sha256(("\n".join(rows) + "\n").encode("utf-8")).hexdigest()
    (EVIDENCE / "copied-files.txt").write_text("\n".join(copied) + "\n", encoding="utf-8")
    (EVIDENCE / "source-manifest.sha256.txt").write_text("\n".join(rows) + "\n", encoding="utf-8")
    summary = {
        "runId": RUN_ID,
        "snapshot": str(SNAPSHOT),
        "copiedFileCount": len(copied),
        "manifestSha256": manifest_hash,
        "verifyStudentsSha256": sha(SOURCE_ROOT / "tools" / "verify_students.py"),
        "m1SolutionSha256": sha(SOURCE_ROOT / "M1_work_stealing_pool" / "solution.cpp"),
        "start": datetime.now(timezone.utc).astimezone().isoformat(),
    }
    (EVIDENCE / "snapshot-summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
