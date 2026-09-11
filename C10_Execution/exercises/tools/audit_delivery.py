"""Audit C10 unit manifest, local Markdown links, isolation wiring, and records."""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile
from typing import Any

sys.dont_write_bytecode = True
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

TOOLS = Path(__file__).resolve().parent
EXERCISES = TOOLS.parent
COURSE = EXERCISES.parent
LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
HEADING = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
EXCLUDE_DIRS = {"build", ".vs", ".cache", "__pycache__", "_deps", "CMakeFiles", "Testing"}


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def slug(title: str) -> str:
    title = re.sub(r"`([^`]*)`", r"\1", title.strip().lower())
    title = re.sub(r"[^\w\u4e00-\u9fff\- ]+", "", title)
    return re.sub(r"\s+", "-", title).strip("-")


def anchors(path: Path) -> set[str]:
    result = set()
    for line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        match = HEADING.match(line)
        if match:
            result.add(slug(match.group(2)))
    return result


def markdown_files(course: Path) -> list[Path]:
    return sorted(p for p in course.rglob("*.md") if not any((part in EXCLUDE_DIRS or part.startswith("build-")) for part in p.relative_to(course).parts))


def markdown_without_code(text: str) -> str:
    lines = []
    fenced = False
    for line in text.splitlines():
        if line.lstrip().startswith("```"):
            fenced = not fenced
            lines.append("")
            continue
        if fenced:
            lines.append("")
        else:
            lines.append(re.sub(r"`[^`]*`", "", line))
    return "\n".join(lines)


def audit_links(course: Path) -> dict[str, Any]:
    failures = []
    checked = 0
    cache: dict[Path, set[str]] = {}
    for md in markdown_files(course):
        text = markdown_without_code(md.read_text(encoding="utf-8-sig", errors="replace"))
        for raw in LINK.findall(text):
            target = raw.split()[0].strip("<>")
            if target.startswith(("http://", "https://", "mailto:")):
                continue
            if target.startswith("#"):
                path = md
                anchor = target[1:]
            else:
                path_part, _, anchor = target.partition("#")
                path = (md.parent / path_part).resolve() if path_part else md
            checked += 1
            if not path.exists():
                failures.append(f"{md.relative_to(course)} -> missing {target}")
                continue
            if anchor:
                cache.setdefault(path, anchors(path))
                if anchor.lower() not in cache[path]:
                    failures.append(f"{md.relative_to(course)} -> missing anchor {target}")
    return {"checked": checked, "failures": failures, "verdict": "FAIL" if failures else "PASS"}


def unit_manifest(build: Path) -> dict[str, Any]:
    path = build / "unit-manifest.json"
    if not path.exists():
        raise RuntimeError(f"missing {path}")
    data = read_json(path)
    if not isinstance(data.get("units"), list):
        raise RuntimeError("unit-manifest.json must contain units[]")
    return data


def audit_units(manifest: dict[str, Any]) -> dict[str, Any]:
    failures = []
    checks: dict[str, int] = {"units": 0, "paths": 0, "variants": 0, "fields": 0}
    names: set[str] = set()
    required = {"name", "source", "header", "student_target", "reference_target",
                "good_target", "bad_target", "bad_diagnostic", "kind"}
    for unit in manifest["units"]:
        checks["units"] += 1
        name = unit.get("name", "")
        if not name or name in names:
            failures.append(f"duplicate or empty unit name: {name!r}")
        names.add(name)
        missing = sorted(required - set(unit))
        if missing:
            failures.append(f"{name}: missing fields {missing}")
        checks["fields"] += len(required) - len(missing)
        source = Path(unit.get("source", ""))
        if not source.is_absolute() or not source.exists():
            failures.append(f"{name}: source must be an existing absolute path")
        else:
            checks["paths"] += 1
            expected = [unit.get("check_source", "main.cpp")]
            if unit.get("kind") != "observation":
                expected += ["src/student/solution.hpp", "src/reference/solution.hpp",
                             "validation/good/solution.hpp", "validation/bad/solution.hpp"]
                reference = source / "src/reference" / unit.get("header", "solution.hpp")
                good = source / "validation/good" / unit.get("header", "solution.hpp")
                if reference.is_file() and good.is_file() and reference.read_bytes() == good.read_bytes():
                    failures.append(f"{name}: independent good is byte-identical to Reference")
            for rel in expected:
                if rel and not (source / rel).exists():
                    failures.append(f"{name}: missing {rel}")
                else:
                    checks["variants"] += 1
        if unit.get("kind") not in {"implementation", "observation"}:
            failures.append(f"{name}: kind must be implementation or observation")
        if unit.get("kind") == "implementation" and not unit.get("bad_diagnostic"):
            failures.append(f"{name}: implementation unit needs bad_diagnostic")
    return {"verdict": "FAIL" if failures else "PASS", "failures": failures, "checks": checks,
            "unit_names": sorted(names)}


def reply_targets(build: Path, config: str) -> dict[str, dict[str, Any]]:
    reply = build / ".cmake/api/v1/reply"
    indexes = sorted(reply.glob("index-*.json"))
    if not indexes:
        raise RuntimeError(f"missing CMake file-api index under {reply}")
    index = read_json(indexes[-1])
    model_ref = next((x for x in index.get("objects", []) if x.get("kind") == "codemodel"), None)
    if not model_ref:
        raise RuntimeError("missing codemodel")
    model = read_json(reply / model_ref["jsonFile"])
    configs = model.get("configurations", [])
    selected = next((c for c in configs if c.get("name") == config), None) or (configs[0] if configs else None)
    if not selected:
        raise RuntimeError("codemodel has no configurations")
    result = {}
    for target in selected.get("targets", []):
        data = read_json(reply / target["jsonFile"])
        result[data["name"]] = data
    return result


def target_inputs(data: dict[str, Any]) -> list[str]:
    values = [s.get("path", "") for s in data.get("sources", [])]
    values += [i.get("path", "") for group in data.get("compileGroups", []) for i in group.get("includes", [])]
    values += [f.get("fragment", "") for f in data.get("link", {}).get("commandFragments", [])]
    return [v for v in values if v]


def audit_isolation(build: Path, config: str, manifest: dict[str, Any]) -> dict[str, Any]:
    failures = []
    targets = reply_targets(build, config)
    checked = 0
    for unit in manifest["units"]:
        if unit.get("kind") == "observation":
            continue
        for key in ("student_target", "good_target"):
            target = unit.get(key)
            data = targets.get(target)
            if not data:
                continue
            checked += 1
            inputs = "\n".join(target_inputs(data)).replace("\\", "/").lower()
            if "/src/reference/" in inputs:
                failures.append(f"{target}: references src/reference input")
    return {"verdict": "FAIL" if failures else "PASS", "failures": failures, "checked_targets": checked}


def summarize_records(paths: list[Path]) -> dict[str, Any]:
    counts: dict[str, int] = {}
    records = []
    failures = []
    for root in paths:
        for path in sorted(root.rglob("*.json")):
            try:
                data = read_json(path)
            except json.JSONDecodeError as error:
                verdict = "MALFORMED"
                data = {"error": str(error)}
            else:
                if not isinstance(data, dict) or ("verdict" not in data and "status" not in data):
                    continue
                verdict = str(data.get("verdict", data.get("status", "UNKNOWN")))
            counts[verdict] = counts.get(verdict, 0) + 1
            item = {"path": str(path), "verdict": verdict, "name": data.get("name"),
                    "exit_code": data.get("exit_code"), "timeout": data.get("timeout")}
            records.append(item)
            if verdict in {"FAIL", "UNKNOWN", "MALFORMED"}:
                failures.append(item)
    return {"verdict": "FAIL" if failures else "PASS", "counts": counts, "records": records,
            "failures": failures}


def audit_coverage_csv(path: Path | None) -> dict[str, Any]:
    if not path:
        return {"verdict": "SKIP", "reason": "no coverage csv supplied",
                "expected_fields": ["old_id", "owner", "exercise", "evidence", "status"]}
    rows = list(csv.DictReader(path.read_text(encoding="utf-8-sig").splitlines()))
    required = {"old_id", "owner", "exercise", "evidence", "status"}
    failures = []
    if set(rows[0].keys() if rows else []) < required:
        failures.append(f"coverage csv must include {sorted(required)}")
    return {"verdict": "FAIL" if failures else "PASS", "failures": failures, "rows": len(rows)}


def file_manifest(course: Path) -> dict[str, Any]:
    digest = hashlib.sha256()
    count = 0
    for path in sorted(p for p in course.rglob("*") if p.is_file()):
        rel = path.relative_to(course)
        if any((part in EXCLUDE_DIRS or part.startswith("build-")) for part in rel.parts):
            continue
        digest.update(hashlib.sha256(path.read_bytes()).hexdigest().encode("ascii") + b"  " +
                      rel.as_posix().encode("utf-8") + b"\n")
        count += 1
    return {"file_count": count, "sha256": digest.hexdigest()}


def run(args: argparse.Namespace) -> dict[str, Any]:
    manifest = unit_manifest(args.build)
    units = audit_units(manifest)
    links = audit_links(args.course)
    isolation = audit_isolation(args.build, args.config, manifest)
    records = summarize_records(args.records)
    coverage = audit_coverage_csv(args.coverage_csv)
    failures = units["failures"] + links["failures"] + isolation["failures"] + records["failures"] + coverage.get("failures", [])
    return {"verdict": "FAIL" if failures else "PASS", "failures": failures, "course": str(args.course.resolve()),
            "unit_manifest": units, "markdown_links": links, "isolation": isolation, "records": records,
            "coverage_csv": coverage, "file_manifest": file_manifest(args.course),
            "limits": "Navigation and wiring checks do not prove teaching quality; non-author review still required."}


def self_check() -> int:
    with tempfile.TemporaryDirectory(prefix="c10-audit-delivery-self-") as d:
        root = Path(d)
        course = root / "C10_Execution"
        unit = course / "exercises/U1"
        build = root / "build"
        for rel in ["src/student", "src/reference", "validation/good", "validation/bad"]:
            (unit / rel).mkdir(parents=True)
            (unit / rel / "solution.hpp").write_text(f"// {rel}\n", encoding="utf-8")
        (unit / "main.cpp").write_text("int main(){return 0;}\n", encoding="utf-8")
        (course / "README.md").write_text("# Root\n[unit](exercises/U1/README.md#task)\n", encoding="utf-8")
        (unit / "README.md").write_text("# Task\n", encoding="utf-8")
        manifest = {"units": [{"name": "U1", "source": str(unit.resolve()), "header": "solution.hpp",
                               "student_target": "U1_student", "reference_target": "U1_reference",
                               "good_target": "U1_validation_good", "bad_target": "U1_validation_bad",
                               "bad_diagnostic": "reject", "kind": "implementation",
                               "check_source": "main.cpp"}]}
        write_json(build / "unit-manifest.json", manifest)
        reply = build / ".cmake/api/v1/reply"
        reply.mkdir(parents=True)
        write_json(reply / "index-1.json", {"objects": [{"kind": "codemodel", "jsonFile": "codemodel.json"}]})
        write_json(reply / "codemodel.json", {"configurations": [{"name": "Release", "targets": [
            {"jsonFile": "student.json"}, {"jsonFile": "good.json"}]}]})
        write_json(reply / "student.json", {"name": "U1_student", "sources": [{"path": str(unit / "src/student/solution.hpp")}]})
        write_json(reply / "good.json", {"name": "U1_validation_good", "sources": [{"path": str(unit / "validation/good/solution.hpp")}]})
        class Args:
            pass
        args = Args()
        args.course = course
        args.build = build
        args.config = "Release"
        args.records = []
        args.coverage_csv = None
        result = run(args)
        assert result["verdict"] == "PASS", result
        (unit / "validation/good/solution.hpp").write_bytes((unit / "src/reference/solution.hpp").read_bytes())
        assert audit_units(manifest)["verdict"] == "FAIL", "copied Reference must not count as independent good"
    print("audit_delivery self-check PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--course", type=Path, default=COURSE)
    parser.add_argument("--build", type=Path)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--records", type=Path, action="append", default=[])
    parser.add_argument("--coverage-csv", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    if args.build is None:
        parser.error("--build is required unless --self-check is used")
    try:
        result = run(args)
    except Exception as error:
        result = {"verdict": "FAIL", "failures": [str(error)]}
    if args.output:
        if args.output.exists():
            parser.error("choose a new output path")
        write_json(args.output, result)
    print(f"{result['verdict']}: audit_delivery")
    for failure in result.get("failures", []):
        print(failure)
    return int(result["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
