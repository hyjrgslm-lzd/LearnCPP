"""Record the current delivery inputs without builds, user work, or review self-hashes."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import subprocess

REPO = Path(__file__).resolve().parents[3]
PROTECTED = {"CONTENT_REFACTORING_GUIDE.md", "Coroutine_Study/exercises/P2_generator_basics/main.cpp"}
COURSES = ("Engineering_Study/", "Coroutine_Study/", "Concurrency_Study/")


def git_paths(*args: str) -> set[str]:
    result = subprocess.run(["git", *args, "-z"], cwd=REPO, check=True, capture_output=True, timeout=30)
    return {p.decode("utf-8") for p in result.stdout.split(b"\0") if p}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path, help="new manifest directory")
    output = parser.parse_args().output.resolve()
    if output.exists():
        parser.error("output must be a new directory")
    visible = git_paths("ls-files", "--cached", "--others", "--exclude-standard")
    changed = git_paths("diff", "--name-only", "HEAD") | git_paths("ls-files", "--others", "--exclude-standard")
    source, evidence = [], []
    for name in sorted(visible):
        path = REPO / name
        if not path.is_file() or ".omx" in path.parts or name in PROTECTED:
            continue
        if name not in {"README.md", "LEARNCPP_GLOBAL_PLAN.md"} and not name.startswith(COURSES):
            continue
        if not name.startswith("Engineering_Study/") and name not in changed:
            continue
        if "/references/validation/" in name:
            # Review reports bind the manifests themselves; never hash that cycle.
            if "review" in path.name.lower() and path.suffix == ".md":
                continue
            if path.suffix == ".sha256" or "snapshots" in path.parts:
                continue
            bucket = evidence
        else:
            bucket = source
        bucket.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n")
    output.mkdir(parents=True)
    for name, lines in (("source.sha256", source), ("evidence.sha256", evidence)):
        (output / name).write_text("".join(lines), encoding="utf-8")
        print(f"{name}: {len(lines)} files; sha256={hashlib.sha256((output / name).read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
