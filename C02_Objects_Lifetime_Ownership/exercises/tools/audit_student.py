"""Audit declared Reference dependencies, actual includes and local source wiring.

Requires a successful --clean-first --target <all students> build with MSVC /showIncludes (English/Chinese)
or Clang/GCC -H, recorded by record_process.py. Request codemodel-v2 before
configuration. Schema: https://cmake.org/cmake/help/v4.2/manual/cmake-file-api.7.html
This detects the course's declared answer paths, not arbitrary copied algorithms.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
REPO = Path(__file__).resolve().parents[3]
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"\n]+)[>"]', re.MULTILINE)


def reference_path(value: str) -> bool:
    return "reference" in value.replace("\\", "/").lower().split("/")


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def audit(build: Path, config: str, trace_path: Path, expected: list[str]):
    failures, inputs, visited = [], {}, set()
    reply = build / ".cmake/api/v1/reply"
    indexes = sorted(reply.glob("index-*.json"))
    if not indexes:
        raise ValueError("missing file-api reply; request codemodel-v2 and configure first")
    index = read_json(indexes[-1])
    refs = [entry for entry in index["objects"] if entry["kind"] == "codemodel"
            and entry["version"]["major"] == 2]
    if not refs:
        raise ValueError("codemodel v2 was not returned")
    model = read_json(reply / refs[0]["jsonFile"])
    source_root = Path(model["paths"]["source"])
    configurations = [item for item in model["configurations"] if item["name"] == config]
    if len(configurations) != 1:
        raise ValueError(f"configuration {config!r} absent or ambiguous")
    targets = {item["id"]: read_json(reply / item["jsonFile"])
               for item in configurations[0]["targets"]}
    students = [target for target in targets.values() if target["name"].endswith("_student")]
    if not students:
        failures.append("no Student targets present")
    names = {target["name"] for target in students}
    for name in expected:
        if name not in names:
            failures.append(f"required Student target absent: {name}")
    for target in targets.values():
        if "_reference" in target["name"].lower():
            failures.append(f"Reference target in Student-only build: {target['name']}")

    def inspect_file(path: Path, include_dirs: list[Path]):
        path = path.resolve()
        if reference_path(str(path)):
            failures.append(f"Reference source/include: {path}")
        key = (path, tuple(include_dirs))
        if key in visited or not path.is_file() or not path.is_relative_to(REPO):
            return
        visited.add(key)
        inputs[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        for header in INCLUDE.findall(path.read_text(encoding="utf-8-sig", errors="replace")):
            if reference_path(header):
                failures.append(f"Reference include spelling in {path}: {header}")
            candidate = next((root / header for root in [path.parent, *include_dirs]
                              if (root / header).is_file()), None)
            if candidate is not None:
                inspect_file(candidate, include_dirs)

    pending = [target["id"] for target in students]
    seen_targets = set()
    while pending:
        identity = pending.pop()
        if identity in seen_targets:
            continue
        seen_targets.add(identity)
        target = targets.get(identity)
        if target is None:
            failures.append(f"unresolved build dependency: {identity}")
            continue
        pending.extend(entry["id"] for entry in target.get("dependencies", []))
        includes = [Path(entry["path"]) for group in target.get("compileGroups", [])
                    for entry in group.get("includes", [])]
        for path in includes:
            if reference_path(str(path)):
                failures.append(f"Reference search directory: {target['name']}: {path}")
        for fragment in target.get("link", {}).get("commandFragments", []):
            value = fragment["fragment"].replace("\\", "/").lower()
            if "/reference/" in value or "_reference" in value:
                failures.append(f"Reference link fragment: {target['name']}: {value}")
        for source in target.get("sources", []):
            path = Path(source["path"])
            inspect_file(path if path.is_absolute() else source_root / path, includes)

    trace = read_json(trace_path)
    if trace.get("exit_code") != 0 or trace.get("timeout") or trace.get("error") or trace.get("cleanup_error"):
        failures.append("include-trace build did not successfully complete")
    command = trace.get("command", [])
    trace_cwd = Path(trace["cwd"])
    if "--build" not in command or (trace_cwd / command[command.index("--build") + 1]).resolve() != build.resolve():
        failures.append("include trace was not recorded for this build directory")
    if "--clean-first" not in command or "--target" not in command or not names.issubset(command):
        failures.append("trace must rebuild every Student target explicitly with --clean-first")
    if index["cmake"]["generator"]["multiConfig"] and (
        "--config" not in command or command[command.index("--config") + 1] != config
    ):
        failures.append("include-trace configuration does not match the codemodel configuration")
    included = []
    for line in (trace.get("stdout", "") + "\n" + trace.get("stderr", "")).splitlines():
        match = re.search(r"(?:including file|包含文件):\s*(.+)$", line, re.IGNORECASE)
        if not match:
            match = re.match(r"^\.+\s+(.+)$", line)
        if match:
            included.append(match.group(1).strip().strip('"'))
    if not included:
        failures.append("no actual preprocessor include trace; a no-op build is insufficient")
    for path in included:
        if reference_path(path):
            failures.append(f"Reference in actual include trace: {path}")
    return {"verdict": "FAIL" if failures else "PASS", "failures": sorted(set(failures)),
            "students": sorted(names), "configuration": config, "build": str(build.resolve()),
            "actual_include_count": len(included), "source_sha256": inputs,
            "trace_sha256": hashlib.sha256(trace_path.read_bytes()).hexdigest(),
            "scope": "declared answer targets/paths, recorded includes and literal local includes; not plagiarism proof"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--expect-target", action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("choose a new evidence filename")
    try:
        result = audit(args.build, args.config, args.trace, args.expect_target)
    except (OSError, ValueError, KeyError, IndexError, TypeError) as error:
        result = {"verdict": "FAIL", "failures": [f"audit input error: {error}"]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write("\n")
    print(result["verdict"] + ": " + str(args.output))
    for failure in result.get("failures", []):
        print(failure)
    return int(result["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
