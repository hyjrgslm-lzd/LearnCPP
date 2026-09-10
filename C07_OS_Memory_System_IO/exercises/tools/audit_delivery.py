"""Audit C07 delivery files, local Markdown links, coverage links, and CTest records."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

sys.dont_write_bytecode = True
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

COURSE = Path(__file__).resolve().parents[2]
LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
HEADING = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
EXCLUDE_DIRS = {"build", ".vs", ".cache", "__pycache__", "_deps", "CMakeFiles", "Testing"}
EXCLUDE_SUFFIXES = {".exe", ".dll", ".lib", ".a", ".so", ".dylib", ".o", ".obj", ".pdb", ".ilk", ".idb"}


def sha256(path: Path) -> str:
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return digest


def slug(title: str) -> str:
    title = re.sub(r"`([^`]*)`", r"\1", title.strip().lower())
    title = re.sub(r"[^\w\u4e00-\u9fff\- ]+", "", title)
    return re.sub(r"\s+", "-", title).strip("-")


def markdown_files(course: Path) -> list[Path]:
    return sorted(p for p in course.rglob("*.md") if not any(part in EXCLUDE_DIRS for part in p.relative_to(course).parts))


def anchors(path: Path) -> set[str]:
    values = set()
    for line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        match = HEADING.match(line)
        if match:
            values.add(slug(match.group(2)))
    return values


def audit_links(course: Path) -> dict:
    failures = []
    checked = 0
    cache: dict[Path, set[str]] = {}
    for md in markdown_files(course):
        for raw in LINK.findall(md.read_text(encoding="utf-8-sig", errors="replace")):
            target = raw.split()[0].strip("<>")
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            path_part, _, anchor = target.partition("#")
            if not path_part:
                target_path = md
            else:
                target_path = (md.parent / path_part).resolve()
            checked += 1
            if not target_path.exists():
                failures.append(f"{md.relative_to(course)} -> missing {target}")
                continue
            if anchor:
                cache.setdefault(target_path, anchors(target_path))
                if anchor.lower() not in cache[target_path]:
                    failures.append(f"{md.relative_to(course)} -> missing anchor {target}")
    return {"checked": checked, "failures": failures, "verdict": "FAIL" if failures else "PASS"}


def delivery_files(course: Path, output: Path | None) -> list[dict]:
    files = []
    output_resolved = output.resolve() if output else None
    for path in sorted(p for p in course.rglob("*") if p.is_file()):
        rel = path.relative_to(course)
        if output_resolved and path.resolve() == output_resolved:
            continue
        if any(part in EXCLUDE_DIRS for part in rel.parts) or rel.suffix.lower() in EXCLUDE_SUFFIXES:
            continue
        files.append({"path": rel.as_posix(), "bytes": path.stat().st_size, "sha256": sha256(path)})
    return files


def audit_coverage(course: Path) -> dict:
    coverage = course / "references/coverage.md"
    if not coverage.exists():
        return {"verdict": "FAIL", "failures": ["references/coverage.md is missing"], "registered_links": []}
    links = sorted(set(LINK.findall(coverage.read_text(encoding="utf-8-sig", errors="replace"))))
    return {"verdict": "PASS", "failures": [], "registered_links": links}


def summarize_records(paths: list[Path]) -> dict:
    counts: dict[str, int] = {}
    records = []
    failures = []
    for root in paths:
        for path in sorted(root.rglob("*.json")):
            try:
                data = json.loads(path.read_text(encoding="utf-8-sig"))
            except json.JSONDecodeError as error:
                counts["MALFORMED"] = counts.get("MALFORMED", 0) + 1
                failures.append({"path": str(path), "name": None, "verdict": "MALFORMED",
                                 "exit_code": None, "timeout": None, "error": str(error)})
                continue
            verdict = str(data.get("verdict", "UNKNOWN"))
            counts[verdict] = counts.get(verdict, 0) + 1
            record = {"path": str(path), "name": data.get("name"), "verdict": verdict,
                      "exit_code": data.get("exit_code"), "timeout": data.get("timeout")}
            records.append(record)
            if verdict in {"FAIL", "UNKNOWN"}:
                failures.append(record)
    return {"counts": counts, "records": records, "failures": failures,
            "verdict": "FAIL" if failures else "PASS"}


def run(course: Path, output: Path | None, records: list[Path]) -> dict:
    link_result = audit_links(course)
    coverage = audit_coverage(course)
    manifest = delivery_files(course, output)
    ctest = summarize_records(records)
    failures = link_result["failures"] + coverage["failures"] + ctest["failures"]
    return {
        "verdict": "FAIL" if failures else "PASS",
        "failures": failures,
        "course": str(course.resolve()),
        "markdown_links": link_result,
        "coverage": coverage,
        "manifest": {
            "scope": "byte SHA256 over delivered C07 files; generated manifest output itself excluded",
            "file_count": len(manifest),
            "files": manifest,
        },
        "ctest_records": ctest,
        "limits": "This audits local links, registered coverage links, file fingerprints, and run_test JSON records; it is not an automatic teaching review.",
    }


def self_check() -> int:
    with tempfile.TemporaryDirectory(prefix="c07-audit-delivery-") as d:
        course = Path(d)
        (course / "references").mkdir()
        (course / "chapters").mkdir()
        (course / "README.md").write_text("# Root\n[ok](chapters/a.md#目标)\n", encoding="utf-8")
        (course / "chapters/a.md").write_text("# 目标\n", encoding="utf-8")
        (course / "references/coverage.md").write_text("[a](../chapters/a.md)\n", encoding="utf-8")
        result = run(course, course / "manifest.json", [])
        assert result["verdict"] == "PASS"
        assert "manifest.json" not in {item["path"] for item in result["manifest"]["files"]}
        records = course / "records"
        records.mkdir()
        (records / "fail.json").write_text('{"name":"bad-case","verdict":"FAIL","exit_code":1,"timeout":false}\n',
                                           encoding="utf-8")
        (records / "unknown.json").write_text('{"name":"missing-verdict","exit_code":0,"timeout":false}\n',
                                              encoding="utf-8")
        (records / "skip.json").write_text('{"name":"optional","verdict":"SKIP","timeout":false}\n',
                                           encoding="utf-8")
        (records / "malformed.json").write_text("{bad json", encoding="utf-8")
        result = run(course, course / "manifest.json", [records])
        assert result["verdict"] == "FAIL"
        assert result["ctest_records"]["counts"] == {"FAIL": 1, "MALFORMED": 1, "SKIP": 1, "UNKNOWN": 1}
        assert len(result["ctest_records"]["failures"]) == 3
    print("audit_delivery self-check PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--course", type=Path, default=COURSE)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--ctest-records", type=Path, action="append", default=[])
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    result = run(args.course.resolve(), args.output, [p.resolve() for p in args.ctest_records])
    if args.output:
        if args.output.exists():
            parser.error("choose a new output path")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("x", encoding="utf-8") as handle:
            json.dump(result, handle, ensure_ascii=False, indent=2)
            handle.write("\n")
    print(f"{result['verdict']}: {args.course} ({result['manifest']['file_count']} files)")
    for failure in result["failures"]:
        print(failure)
    return int(result["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
