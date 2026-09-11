"""Audit navigable C09 documents and freeze tracked/eligible delivery bytes."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[3]
COURSE = ROOT / "C09_Coroutines"
OUTPUT = COURSE / "references/validation/c09-refresh"
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "C07_OS_Memory_System_IO/exercises/tools"))
from audit_delivery import LINK, slug


def eligible_files():
    result = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z", "--",
                             "C09_Coroutines", "README.md", "LEARNCPP_GLOBAL_PLAN.md"],
                            cwd=ROOT, capture_output=True, check=True)
    return sorted({ROOT / name.decode("utf-8") for name in result.stdout.split(b"\0") if name
                   and (ROOT / name.decode("utf-8")).is_file()})


def visible_markdown(path):
    lines, fence = [], None
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        marker = re.match(r"\s*(```|~~~)", line)
        if marker:
            fence = None if fence == marker[1] else marker[1]
        elif fence is None:
            lines.append(line)
    return "\n".join(lines)


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    files = eligible_files()
    documents = [p for p in files if p.suffix == ".md" and p.is_relative_to(COURSE)
                 and "validation" not in p.relative_to(COURSE).parts]
    failures, count, cache = [], 0, {}
    for doc in documents:
        for raw in LINK.findall(visible_markdown(doc)):
            raw = raw.strip()
            target = raw[1:raw.find(">")] if raw.startswith("<") else raw.split(' "', 1)[0]
            if target.startswith(("https://", "http://", "mailto:")):
                continue
            path, _, anchor = unquote(target).partition("#")
            destination = (doc.parent / path).resolve() if path else doc
            count += 1
            if not destination.exists():
                failures.append(f"{doc.relative_to(ROOT)} -> missing {target}")
            elif anchor and destination.suffix == ".md":
                if destination not in cache:
                    text = visible_markdown(destination)
                    cache[destination] = {slug(x) for x in re.findall(r"^#{1,6}\s+(.+)$", text, re.M)}
                    cache[destination].update(re.findall(r'<a\s+(?:id|name)=["\x27]([^"\x27]+)', text))
                if anchor.lower() not in cache[destination]:
                    failures.append(f"{doc.relative_to(ROOT)} -> missing anchor {target}")
    coverage = (COURSE / "references/coverage.md").read_text(encoding="utf-8-sig")
    registration = coverage.split("## 37 单元", 1)[1].split("\n## ", 1)[0]
    units = re.findall(r"^\| (P[12]|[A-J][1-5]|Capstone[145]) \|", registration, re.M)
    if len(units) != 37 or len(set(units)) != 37:
        failures.append(f"coverage must register 37 unique units, found {len(units)}/{len(set(units))}")
    audit = {"documents": len(documents), "local_links": count, "units": len(set(units)),
             "failures": failures, "verdict": "FAIL" if failures else "PASS"}
    excluded = {OUTPUT / "delivery-manifest.json", OUTPUT / "delivery-audit.json",
                OUTPUT / "reviews/final-integration-review.md"}
    entries = []
    for path in eligible_files():
        if path in excluded:
            continue
        content = path.read_bytes()
        if content[:4] == b"\x7fELF" or content[:2] == b"MZ":
            failures.append(f"binary build artifact is eligible for delivery: {path.relative_to(ROOT)}")
        entries.append({"path": path.relative_to(ROOT).as_posix(), "bytes": len(content),
                        "sha256": hashlib.sha256(content).hexdigest()})
    (OUTPUT / "delivery-manifest.json").write_text(json.dumps({
        "files": entries, "excludes": [p.relative_to(ROOT).as_posix() for p in sorted(excluded)],
        "note": "historical ignored executables remain local; final attestation is outside its own manifest"
    }, ensure_ascii=False, indent=2), encoding="utf-8")
    audit["verdict"] = "FAIL" if failures else "PASS"
    (OUTPUT / "delivery-audit.json").write_text(json.dumps(audit, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(audit, ensure_ascii=False, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
