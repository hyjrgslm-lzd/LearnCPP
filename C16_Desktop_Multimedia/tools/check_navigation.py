"""Check C16 Markdown links; root README only gets the new C16 entry check."""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
HEADING = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
EXCLUDE_DIRS = {"build", ".vs", ".cache", "__pycache__", "_deps", "CMakeFiles", "Testing"}


def links(path: Path) -> list[str]:
    found: list[str] = []
    fenced = False
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if line.lstrip().startswith("```"):
            fenced = not fenced
            continue
        if not fenced:
            found.extend(raw.split()[0].strip("<>") for raw in LINK.findall(line))
    return found


def slug(title: str) -> str:
    title = re.sub(r"`([^`]*)`", r"\1", title.strip().lower())
    title = re.sub(r"[^\w\u4e00-\u9fff\- ]+", "", title)
    return re.sub(r"\s+", "-", title).strip("-")


def anchors(path: Path) -> set[str]:
    values = set()
    for line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        match = HEADING.match(line)
        if match:
            values.add(slug(match.group(2)))
    return values


def markdown_files(course: Path) -> list[Path]:
    return sorted(
        p for p in course.rglob("*.md")
        if not any(part in EXCLUDE_DIRS for part in p.relative_to(course).parts)
    )


def check_markdown_links(course: Path) -> list[str]:
    failures: list[str] = []
    anchor_cache: dict[Path, set[str]] = {}
    for md in markdown_files(course):
        rel = md.relative_to(course).as_posix()
        for target in links(md):
            if target.startswith(("http://", "https://", "mailto:")):
                continue
            path_part, _, anchor = target.partition("#")
            target_path = md if not path_part else (md.parent / path_part).resolve()
            if path_part and not target_path.exists():
                failures.append(f"{rel} -> missing {target}")
                continue
            if anchor:
                anchor_cache.setdefault(target_path, anchors(target_path))
                if anchor.lower() not in anchor_cache[target_path]:
                    failures.append(f"{rel} -> missing anchor {target}")
    return failures


def check(root: Path) -> list[str]:
    course = root / "C16_Desktop_Multimedia"
    failures: list[str] = []
    if not course.exists():
        failures.append("missing C16_Desktop_Multimedia/")
    if not (course / "README.md").exists():
        failures.append("missing C16_Desktop_Multimedia/README.md")
    root_readme = root / "README.md"
    if not root_readme.exists():
        failures.append("missing README.md")
        return failures
    expected = "C16_Desktop_Multimedia/README.md"
    if expected not in links(root_readme):
        failures.append(f"README.md missing link {expected}")
    if expected in links(root_readme) and not (root / expected).exists():
        failures.append(f"README.md link target missing {expected}")
    if course.exists():
        failures.extend(check_markdown_links(course))
    return failures


def self_check() -> int:
    import tempfile

    with tempfile.TemporaryDirectory(prefix="c16-nav-") as temp:
        root = Path(temp)
        (root / "C16_Desktop_Multimedia").mkdir()
        (root / "shared.md").write_text("# 外部锚点\n", encoding="utf-8")
        (root / "README.md").write_text("[C16](C16_Desktop_Multimedia/README.md)\n", encoding="utf-8")
        (root / "C16_Desktop_Multimedia/README.md").write_text(
            "# 入口\n[tool](tools/run_check.py)\n[chapter](chapters/a.md#目标)\n[outer](../shared.md#外部锚点)\n"
            "```cpp\nconnect(worker, &Worker::done, receiver, [receiver](Result r) {});\n```\n",
            encoding="utf-8")
        (root / "C16_Desktop_Multimedia/chapters").mkdir()
        (root / "C16_Desktop_Multimedia/chapters/a.md").write_text("# 目标\n[back](../README.md#入口)\n",
                                                                     encoding="utf-8")
        (root / "C16_Desktop_Multimedia/build").mkdir()
        (root / "C16_Desktop_Multimedia/build/ignored.md").write_text("[missing](nope.md)\n", encoding="utf-8")
        (root / "C16_Desktop_Multimedia/tools").mkdir()
        (root / "C16_Desktop_Multimedia/tools/run_check.py").write_text("", encoding="utf-8")
        assert check(root) == []
        (root / "README.md").write_text("no c16\n", encoding="utf-8")
        assert check(root) == ["README.md missing link C16_Desktop_Multimedia/README.md"]
        (root / "README.md").write_text("[C16](C16_Desktop_Multimedia/README.md)\n", encoding="utf-8")
        (root / "C16_Desktop_Multimedia/chapters/a.md").write_text("# 目标\n[bad](../README.md#没有)\n",
                                                                     encoding="utf-8")
        assert check(root) == ["chapters/a.md -> missing anchor ../README.md#没有"]
    print("check_navigation self-check PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    failures = check(args.root.resolve())
    if failures:
        print("check_navigation: FAIL", file=sys.stderr)
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1
    print("check_navigation: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
