"""Verify C10 Student rejection, independent good overlays, and bad rejection."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import uuid
from typing import Any

sys.dont_write_bytecode = True
# Avoid accumulating detached MSBuild worker nodes across many independent configurations.
os.environ.setdefault("MSBUILDDISABLENODEREUSE", "1")
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

TOOLS = Path(__file__).resolve().parent
EXERCISES = TOOLS.parent
COURSE = EXERCISES.parent
REPO = COURSE.parent
sys.path.insert(0, str(REPO / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process  # noqa: E402


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def record(records: Path | None, name: str, data: dict[str, Any]) -> None:
    if not records:
        return
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    write_json(records / f"{name}-{stamp}.json", data)


def cmake_cache(build: Path) -> dict[str, str]:
    cache = build / "CMakeCache.txt"
    values: dict[str, str] = {}
    if not cache.exists():
        return values
    for line in cache.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        key_type, value = line.split("=", 1)
        key = key_type.split(":", 1)[0]
        values[key] = value
    return values


def codemodel(build: Path) -> tuple[Path, dict[str, Any], dict[str, Any]]:
    reply = build / ".cmake/api/v1/reply"
    indexes = sorted(reply.glob("index-*.json"))
    if not indexes:
        raise RuntimeError(f"missing CMake file-api index under {reply}")
    index = read_json(indexes[-1])
    refs = [x for x in index.get("objects", []) if x.get("kind") == "codemodel" and x.get("version", {}).get("major") == 2]
    if not refs:
        raise RuntimeError("missing codemodel-v2 reply")
    model = read_json(reply / refs[0]["jsonFile"])
    return reply, index, model


def config_targets(build: Path, config: str) -> dict[str, dict[str, Any]]:
    reply, _, model = codemodel(build)
    configurations = model.get("configurations", [])
    selected = next((c for c in configurations if c.get("name") == config), None)
    selected = selected or (configurations[0] if configurations else None)
    if not selected:
        raise RuntimeError("codemodel has no configurations")
    result = {}
    for item in selected.get("targets", []):
        data = read_json(reply / item["jsonFile"])
        result[data["name"]] = data
    return result


def target_artifact(build: Path, config: str, target: str) -> Path:
    targets = config_targets(build, config)
    data = targets.get(target)
    if not data:
        raise RuntimeError(f"target {target} missing in codemodel")
    artifacts = data.get("artifacts", [])
    if not artifacts:
        raise RuntimeError(f"target {target} has no artifact")
    path = Path(artifacts[0]["path"])
    return path if path.is_absolute() else (build / path).resolve()


def target_closure(build: Path, config: str, roots: list[str]) -> list[dict[str, Any]]:
    targets = config_targets(build, config)
    pending = list(roots)
    seen: set[str] = set()
    found: list[dict[str, Any]] = []
    by_id = {data.get("id"): data for data in targets.values()}
    while pending:
        name_or_id = pending.pop()
        data = targets.get(name_or_id) or by_id.get(name_or_id)
        if not data or data.get("id") in seen:
            continue
        seen.add(data["id"])
        found.append(data)
        pending.extend(dep.get("id") for dep in data.get("dependencies", []))
    return found


def normalize(text: str) -> str:
    return text.replace("\\", "/").lower()


def target_inputs(target: dict[str, Any]) -> list[str]:
    values = [s.get("path", "") for s in target.get("sources", [])]
    values += [inc.get("path", "") for group in target.get("compileGroups", []) for inc in group.get("includes", [])]
    values += [frag.get("fragment", "") for frag in target.get("link", {}).get("commandFragments", [])]
    return [v for v in values if v]


def assert_independent(build: Path, config: str, target: str, good_root: Path, build_result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    good_norm = normalize(str(good_root.resolve()))
    saw_good = False
    for data in target_closure(build, config, [target]):
        name = data.get("name", "")
        if name.endswith("_reference") or "_reference" in name:
            failures.append(f"{target}: depends on Reference target {name}")
        for value in target_inputs(data):
            n = normalize(value)
            if "/src/reference/" in n or "/validation/good/" in n or "/validation/bad/" in n:
                failures.append(f"{target}: Reference input {value}")
            if good_norm in n:
                saw_good = True
    combined = build_result.get("stdout", "") + "\n" + build_result.get("stderr", "")
    includes = []
    for line in combined.splitlines():
        match = re.search(r"(?:including file|鍖呭惈鏂囦欢):\s*(.+)$", line, re.I)
        if match:
            includes.append(match.group(1).strip())
        stripped = line.lstrip(". ").strip()
        if stripped.startswith(str(good_root)) or normalize(str(good_root)) in normalize(stripped):
            includes.append(stripped)
    if includes:
        if any(any(bad in normalize(item) for bad in ("/src/reference/", "/validation/good/", "/validation/bad/"))
               for item in includes):
            failures.append(f"{target}: actual include trace references repository answer path")
        if any(good_norm in normalize(item) for item in includes):
            saw_good = True
    if not saw_good:
        failures.append(f"{target}: no codemodel/include evidence that subject came from good overlay")
    return sorted(set(failures))


def cmake_configure_command(source: Path, build: Path, config: str, args: argparse.Namespace,
                            student_root: Path | None, units: list[str] | None,
                            build_reference: bool) -> list[str]:
    command = ["cmake", "-S", str(source), "-B", str(build),
               f"-DC10_STUDY_BUILD_REFERENCE={'ON' if build_reference else 'OFF'}",
               "-DC10_STUDY_TEST_STUDENTS=ON", "-DC10_STUDY_TRACE_INCLUDES=ON",
               f"-DCMAKE_BUILD_TYPE={config}"]
    if args.stdexec_source:
        command.append(f"-DFETCHCONTENT_SOURCE_DIR_STDEXEC={args.stdexec_source}")
    if args.cxx_standard:
        command.append(f"-DC10_CXX_STANDARD={args.cxx_standard}")
    if args.compiler:
        command.append(f"-DCMAKE_CXX_COMPILER={args.compiler}")
    if args.io_uring:
        command.append("-DC10_STUDY_ENABLE_IO_URING=ON")
    if args.liburing_root:
        command.append(f"-DC10_LIBURING_ROOT={args.liburing_root}")
    if student_root:
        command.append(f"-DC10_STUDY_STUDENT_ROOT={student_root}")
    if units:
        command.append(f"-DC10_UNITS={';'.join(units)}")
    return command


def ensure_file_api_query(build: Path) -> None:
    query = build / ".cmake/api/v1/query"
    query.mkdir(parents=True, exist_ok=True)
    (query / "codemodel-v2").write_text("", encoding="utf-8")


def build_target(build: Path, config: str, target: str, timeout: float) -> dict[str, Any]:
    return run_process(["cmake", "--build", str(build), "--config", config, "--target", target],
                       timeout)


def run_artifact(build: Path, config: str, target: str, timeout: float) -> dict[str, Any]:
    return run_process([str(target_artifact(build, config, target))], timeout)


def copy_overlay(unit: dict[str, Any], variant: str, root: Path) -> Path:
    source_unit = Path(unit["source"])
    src = source_unit / "validation" / variant
    if not (src / "solution.hpp").exists():
        raise RuntimeError(f"{unit['name']}: missing {src / 'solution.hpp'}")
    dst = root / unit["name"]
    shutil.copytree(src, dst, dirs_exist_ok=True)
    return dst


def verify_unit(unit: dict[str, Any], source: Path, args: argparse.Namespace, records: Path | None) -> dict[str, Any]:
    name = unit["name"]
    failures: list[str] = []
    out: dict[str, Any] = {"name": name, "kind": unit.get("kind", "implementation")}
    if unit.get("kind") == "observation":
        out.update(verdict="PASS", failures=[], skipped="observation unit has no Student implementation")
        return out
    student_build = build_target(args.build_dir, args.config, unit["student_target"], args.timeout)
    student_run = run_artifact(args.build_dir, args.config, unit["student_target"], args.timeout) if student_build["status"] == "PASS" else {}
    text = student_run.get("stdout", "") + "\n" + student_run.get("stderr", "")
    expected_exit = unit.get("student_expected_exit", 2)
    expected_diagnostic = unit.get("student_expected_diagnostic", "UNFINISHED")
    if expected_exit not in (1, 2) or not expected_diagnostic:
        failures.append(f"{name}: invalid initial Student failure contract")
    if student_build["status"] != "PASS":
        failures.append(f"{name}: starter student build failed")
    elif not (student_run.get("exit_code") == expected_exit and expected_diagnostic in text
              and not student_run.get("timeout") and not student_run.get("cleanup_error")
              and not student_run.get("error")):
        failures.append(f"{name}: starter must exit {expected_exit} with {expected_diagnostic!r}")
    out["starter_build"] = student_build
    out["starter_run"] = student_run
    record(records, f"{name}-starter-build", student_build)
    if student_run:
        record(records, f"{name}-starter-run", student_run)

    with tempfile.TemporaryDirectory(prefix=f"c10-{name}-good-") as temp:
        overlay = Path(temp) / "students"
        unit_overlay = copy_overlay(unit, "good", overlay)
        # Keep MSBuild scratch paths below its legacy path limit, independently of evidence location.
        good_build = REPO / "build" / f"c10-good-{uuid.uuid4().hex[:8]}"
        ensure_file_api_query(good_build)
        cfg = run_process(cmake_configure_command(source, good_build, args.config, args, overlay, [name], False), args.timeout)
        good_target = unit["student_target"]
        build = build_target(good_build, args.config, good_target, args.timeout) if cfg["status"] == "PASS" else {}
        run = run_artifact(good_build, args.config, good_target, args.timeout) if build.get("status") == "PASS" else {}
        if cfg["status"] != "PASS":
            failures.append(f"{name}: good configure failed")
        elif build.get("status") != "PASS":
            failures.append(f"{name}: good build failed")
        elif run.get("status") != "PASS" or run.get("exit_code") != 0 or run.get("timeout") or run.get("cleanup_error"):
            failures.append(f"{name}: good run failed")
        if build:
            failures += assert_independent(good_build, args.config, good_target, unit_overlay, build)
        out["good"] = {"configure": cfg, "build": build, "run": run, "build_dir": str(good_build)}
        record(records, f"{name}-good-configure", cfg)
        if build:
            record(records, f"{name}-good-build", build)
        if run:
            record(records, f"{name}-good-run", run)

    with tempfile.TemporaryDirectory(prefix=f"c10-{name}-bad-") as temp:
        overlay = Path(temp) / "students"
        copy_overlay(unit, "bad", overlay)
        bad_build = REPO / "build" / f"c10-bad-{uuid.uuid4().hex[:8]}"
        ensure_file_api_query(bad_build)
        cfg = run_process(cmake_configure_command(source, bad_build, args.config, args, overlay, [name], False), args.timeout)
        bad_target = unit["student_target"]
        build = build_target(bad_build, args.config, bad_target, args.timeout) if cfg["status"] == "PASS" else {}
        run = run_artifact(bad_build, args.config, bad_target, args.timeout) if build.get("status") == "PASS" else {}
        diagnostic = unit.get("bad_diagnostic", "")
        text = run.get("stdout", "") + "\n" + run.get("stderr", "")
        if cfg["status"] != "PASS":
            failures.append(f"{name}: bad configure failed")
        elif build.get("status") != "PASS":
            failures.append(f"{name}: bad build failed; compile failure is not behavioral rejection")
        elif not (run.get("exit_code") == 1 and diagnostic and diagnostic in text and not run.get("timeout")
                  and not run.get("cleanup_error")):
            failures.append(f"{name}: bad must exit 1 and contain {diagnostic!r}")
        if build:
            failures += assert_independent(bad_build, args.config, bad_target, overlay / name, build)
        out["bad"] = {"configure": cfg, "build": build, "run": run, "build_dir": str(bad_build)}
        record(records, f"{name}-bad-configure", cfg)
        if build:
            record(records, f"{name}-bad-build", build)
        if run:
            record(records, f"{name}-bad-run", run)

    out["verdict"] = "FAIL" if failures else "PASS"
    out["failures"] = failures
    return out


def load_manifest(build: Path) -> dict[str, Any]:
    manifest = build / "unit-manifest.json"
    if not manifest.exists():
        raise RuntimeError(f"missing {manifest}")
    data = read_json(manifest)
    if not isinstance(data.get("units"), list):
        raise RuntimeError("unit-manifest.json must contain units[]")
    return data


def self_check() -> int:
    with tempfile.TemporaryDirectory(prefix="c10-verify-students-self-") as d:
        root = Path(d)
        build = root / "build"
        reply = build / ".cmake/api/v1/reply"
        reply.mkdir(parents=True)
        (reply / "index-1.json").write_text(json.dumps({"objects": [{"kind": "codemodel", "version": {"major": 2},
                                                                      "jsonFile": "codemodel.json"}]}), encoding="utf-8")
        (reply / "codemodel.json").write_text(json.dumps({"configurations": [{"name": "Release", "targets": [
            {"jsonFile": "target.json"}]}]}), encoding="utf-8")
        exe = root / "ok.exe"
        exe.write_text("", encoding="utf-8")
        (reply / "target.json").write_text(json.dumps({"name": "u_validation_good", "id": "1",
                                                        "artifacts": [{"path": str(exe)}],
                                                        "sources": [{"path": str(root / "u/solution.hpp")}]}),
                                            encoding="utf-8")
        assert target_artifact(build, "Release", "u_validation_good") == exe
        assert not assert_independent(build, "Release", "u_validation_good", root / "u",
                                      {"stdout": f"including file: {root / 'u/solution.hpp'}", "stderr": ""})
    print("verify_students self-check PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=EXERCISES / "build")
    parser.add_argument("--source", type=Path, default=EXERCISES)
    parser.add_argument("--stdexec-source", type=Path)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--records", type=Path)
    parser.add_argument("--unit", action="append", default=[])
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--cxx-standard")
    parser.add_argument("--compiler")
    parser.add_argument("--io-uring", action="store_true")
    parser.add_argument("--liburing-root", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    try:
        manifest = load_manifest(args.build_dir)
        wanted = set(args.unit)
        units = [u for u in manifest["units"] if not wanted or u["name"] in wanted]
        if not units:
            raise RuntimeError("unit-manifest.json selected zero units")
        if wanted and len(units) != len(wanted):
            missing = sorted(wanted - {u["name"] for u in units})
            raise RuntimeError(f"manifest missing requested units: {missing}")
        results = [verify_unit(unit, args.source.resolve(), args, args.records) for unit in units]
        failures = [failure for item in results for failure in item.get("failures", [])]
        result = {"verdict": "FAIL" if failures else "PASS", "failures": failures, "units": results,
                  "cache": cmake_cache(args.build_dir),
                  "scope": "declared starter failure, good overlay Reference-off behavior, bad runtime rejection"}
    except Exception as error:
        result = {"verdict": "FAIL", "failures": [str(error)]}
    if args.output:
        if args.output.exists():
            parser.error("choose a new output path")
        write_json(args.output, result)
    print(f"{result['verdict']}: verify_students")
    for failure in result.get("failures", []):
        print(failure)
    return int(result["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
