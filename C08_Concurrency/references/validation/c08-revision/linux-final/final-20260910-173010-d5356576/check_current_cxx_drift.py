from __future__ import annotations

import datetime as dt
import hashlib
import json
from pathlib import Path

ROOT = Path.cwd()
EVIDENCE = ROOT / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / "final-20260910-173010-d5356576"
BASE = ROOT / "C08_Concurrency" / "exercises"
EXCLUDE_DIRS = {".git", ".omx", ".codex", ".agents", ".omc", ".claude", "_deps", "CMakeFiles"}
EXTS = {".cpp", ".hpp", ".h", ".cmake"}
NAMES = {"CMakeLists.txt", "CMakePresets.json"}


def walk_files(root: Path):
    stack = [root]
    while stack:
        current = stack.pop()
        for entry in current.iterdir():
            if entry.is_dir():
                if entry.name.startswith(".") or entry.name in EXCLUDE_DIRS or entry.name.lower().startswith("build"):
                    continue
                stack.append(entry)
            elif entry.is_file():
                yield entry


def main() -> int:
    snapshot_end = dt.datetime.fromisoformat((EVIDENCE / "snapshot-end.txt").read_text(encoding="utf-8").strip())
    files: list[str] = []
    changed: list[dict[str, str]] = []
    manifest_hash = hashlib.sha256()
    for path in walk_files(BASE):
        if path.suffix.lower() not in EXTS and path.name not in NAMES:
            continue
        rel = path.relative_to(ROOT).as_posix()
        files.append(rel)
        mtime = dt.datetime.fromtimestamp(path.stat().st_mtime).astimezone()
        if mtime > snapshot_end:
            changed.append({"path": rel, "lastWriteTime": mtime.isoformat()})
    for rel in sorted(files):
        digest = hashlib.sha256((ROOT / rel).read_bytes()).hexdigest()
        manifest_hash.update(f"{digest}  {rel}\n".encode("utf-8"))
    out = {
        "checkedFiles": len(files),
        "hashOfCurrentCxxManifest": manifest_hash.hexdigest(),
        "snapshotEnd": snapshot_end.isoformat(),
        "filesModifiedAfterSnapshotEnd": changed,
    }
    (EVIDENCE / "current-cxx-drift-after-snapshot.json").write_text(
        json.dumps(out, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(out, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
