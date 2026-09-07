"""Check local course links, anchors, and source/reference/build registration."""
from __future__ import annotations

import argparse
import html
import re
from pathlib import Path
import unicodedata
from urllib.parse import unquote, urlsplit


def prose_lines(text: str):
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        match = re.match(r"^\s{0,3}(`{3,}|~{3,})", line)
        if match:
            marker = match.group(1)
            if fence is None:
                fence = marker
            elif marker[0] == fence[0] and len(marker) >= len(fence):
                fence = None
            continue
        if fence is None:
            yield number, line


def heading_slug(heading: str) -> str:
    heading = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', heading)
    heading = re.sub(r'`([^`]+)`', lambda match: html.escape(match.group(1), quote=False), heading)
    heading = re.sub(r"<[^>]*>", "", heading)
    heading = html.unescape(heading).strip().lower()
    return "".join("-" if character.isspace() else character for character in heading
                   if character.isspace() or character in "_-" or
                   unicodedata.category(character)[0] in "LNM")


def anchors(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8-sig")
    prose = "\n".join(line for _, line in prose_lines(text))
    prose = re.sub(r'`([^`]+)`', lambda match: html.escape(match.group(1), quote=False), prose)
    result = set(re.findall(r'<[A-Za-z][^>]*\b(?:id|name)\s*=["\']([^"\']+)["\']', prose))
    counts: dict[str, int] = {}
    for _, line in prose_lines(text):
        match = re.match(r"^#{1,6}\s+(.+?)(?:\s+#+\s*)?$", line)
        if match:
            base = heading_slug(match.group(1))
            index = counts.get(base, 0)
            counts[base] = index + 1
            result.add(base if index == 0 else f"{base}-{index}")
    return result


def check_document(path: Path, workspace: Path, anchor_cache: dict) -> list[str]:
    errors = []
    text = path.read_text(encoding="utf-8-sig")
    # Code fences are examples, not page navigation. HTTP URLs are checked by
    # source reviewers; this tool does not fetch the network or execute links.
    pattern = re.compile(r"!?\[[^\]]*\]\(\s*(?:<([^>]+)>|([^\s)]+))(?:\s+[\"'][^\"']*[\"'])?\s*\)")
    for number, line in prose_lines(text):
        for match in pattern.finditer(line):
            target = match.group(1) or match.group(2)
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc:
                continue
            raw_path = unquote(parsed.path)
            destination = (path.parent / raw_path).resolve() if raw_path else path.resolve()
            if not destination.is_relative_to(workspace):
                errors.append(f"{path}:{number}: link leaves workspace: {target}")
                continue
            if not destination.exists():
                errors.append(f"{path}:{number}: missing target: {target}")
                continue
            if destination.is_dir():
                destination = destination / "README.md"
            fragment = unquote(parsed.fragment)
            if fragment and destination.suffix.lower() == ".md" and destination.is_file():
                if destination not in anchor_cache:
                    anchor_cache[destination] = anchors(destination)
                if fragment not in anchor_cache[destination]:
                    errors.append(f"{path}:{number}: missing anchor: {target}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--course", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    course = args.course.resolve(strict=True)
    errors, cache = [], {}
    documents = []
    for path in course.rglob("*.md"):
        relative = path.relative_to(course)
        if any(part.startswith(".") or part.startswith("build") or part in {"third_party", "__pycache__"}
               for part in relative.parts):
            continue
        documents.append(path)
        errors.extend(check_document(path, course.parent, cache))
    exercises = []
    for directory in sorted((course / "exercises").iterdir()):
        if directory.is_dir() and not directory.name.startswith("build") and (directory / "main.cpp").is_file():
            exercises.append(directory)
            for filename in ("solution.cpp", "README.md", "CMakeLists.txt"):
                if not (directory / filename).is_file():
                    errors.append(f"{directory}: missing {filename}")
            if (directory / "CMakeLists.txt").is_file():
                cmake = (directory / "CMakeLists.txt").read_text(encoding="utf-8-sig")
                registrations = [f"cs_add_exercise({directory.name} {kind})" in cmake
                                 for kind in ("IMPLEMENTATION", "OBSERVATION")]
                if sum(registrations) != 1:
                    errors.append(f"{directory}: register once with an explicit IMPLEMENTATION/OBSERVATION role")
    for error in errors:
        print(error)
    print(f"documents={len(documents)} exercises={len(exercises)} local_errors={len(errors)}")
    print("This checks navigation and registration, not teaching quality, standard correctness, or test adequacy.")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
