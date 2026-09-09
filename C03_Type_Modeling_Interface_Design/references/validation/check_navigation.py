"""Check shipped Markdown targets/anchors for C03 and its six navigation edits."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit

COURSE = Path(__file__).resolve().parents[2]
REPO = COURSE.parent
LINK = re.compile(r"!?\[[^\]\n]*\]\(([^)\n]+)\)")


def prose(text: str) -> str:
    return re.sub(r"(?ms)^\s*(```|~~~).*?^\s*\1\s*$", "", text)


def anchors(path: Path) -> set[str]:
    body = prose(path.read_text(encoding="utf-8-sig"))
    result = set(re.findall(r'<a\s+(?:id|name)=[\'"]([^\'"]+)', body))
    counts: dict[str, int] = {}
    for title in re.findall(r"(?m)^ {0,3}#{1,6}\s+(.+?)\s*#*\s*$", body):
        title = re.sub(r"\[([^]]+)\]\([^)]+\)", r"\1", title).replace("`", "")
        base = re.sub(r"[^\w\- ]", "", title.lower()).replace(" ", "-")
        number = counts.get(base, 0)
        counts[base] = number + 1
        result.add(base if number == 0 else f"{base}-{number}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("choose a new evidence filename")
    files = sorted(p for p in COURSE.rglob("*.md")
                   if not any(part == "build" or part.startswith("build-") for part in p.parts))
    files += [REPO / "README.md", REPO / "LEARNCPP_GLOBAL_PLAN.md"]
    files += [REPO / folder / "README.md" for folder in (
        "C02_Objects_Lifetime_Ownership", "C06_Ranges", "C09_Coroutines", "C10_Execution")]
    failures, external_local, checked = [], [], 0
    fingerprints = {}
    cached_anchors: dict[Path, set[str]] = {}
    for source in files:
        fingerprints[str(source.relative_to(REPO))] = hashlib.sha256(source.read_bytes()).hexdigest()
        for match in LINK.finditer(prose(source.read_text(encoding="utf-8-sig"))):
            raw = match.group(1).strip().split(' "', 1)[0].strip("<>")
            if re.match(r"^(https?|mailto|app):|^//", raw):
                continue
            if re.match(r"^[A-Za-z]:[/\\]", raw):
                path_text, _, fragment = raw.partition("#")
            else:
                parts = urlsplit(raw)
                path_text, fragment = parts.path, parts.fragment
            target = (source.parent / unquote(path_text)).resolve() if path_text else source
            reference = {"source": str(source.relative_to(REPO)), "target": raw}
            if not target.is_relative_to(REPO):
                external_local.append(reference)
                continue
            checked += 1
            if not target.exists():
                failures.append({**reference, "error": "missing target"})
            elif fragment and target.suffix == ".md":
                if target not in cached_anchors:
                    cached_anchors[target] = anchors(target)
                if unquote(fragment).lower() not in cached_anchors[target]:
                    failures.append({**reference, "error": "missing anchor"})
    completed = subprocess.run(["git", "check-ignore", "--stdin"], cwd=REPO,
        input="\n".join(str(p.relative_to(REPO)).replace("\\", "/") for p in files),
        text=True, encoding="utf-8", capture_output=True, timeout=30)
    if completed.returncode not in (0, 1):
        failures.append({"error": "git check-ignore failed", "stderr": completed.stderr})
    ignored_documents = completed.stdout.splitlines()
    result = {"verdict": "FAIL" if failures or ignored_documents else "PASS",
              "documents": len(files), "local_links_checked": checked,
              "failures": failures, "external_local_links": external_local,
              "ignored_documents": ignored_documents, "sha256": fingerprints,
              "scope": "Markdown targets and heading anchors; not teaching-quality or external-web validation"}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in result.items() if k != "sha256"}, ensure_ascii=False))
    return int(result["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
