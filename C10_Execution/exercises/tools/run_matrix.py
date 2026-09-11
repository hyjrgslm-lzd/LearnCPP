"""Run bounded C10 configure/build/ctest matrices and keep raw records."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import shutil
import sys
import uuid
import xml.etree.ElementTree as ET
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


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def cache_values(build: Path) -> dict[str, str]:
    path = build / "CMakeCache.txt"
    values: dict[str, str] = {}
    if not path.exists():
        return values
    for line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        key_type, value = line.split("=", 1)
        values[key_type.split(":", 1)[0]] = value
    return values


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def load_manifest(build: Path) -> dict[str, Any]:
    path = build / "unit-manifest.json"
    if not path.exists():
        return {"units": []}
    data = read_json(path)
    return data if isinstance(data.get("units"), list) else {"units": []}


def version_of(command: list[str], timeout: float = 10) -> dict[str, Any]:
    exe = shutil.which(command[0])
    result = run_process(command, timeout) if exe else {"status": "FAIL", "error": "not found", "command": command}
    return {"path": exe, "result": result}


def env_record(args: argparse.Namespace) -> dict[str, Any]:
    return {
        "platform": platform.platform(),
        "python": sys.version,
        "cwd": str(Path.cwd()),
        "source": str(args.source.resolve()),
        "stdexec_source": str(args.stdexec_source.resolve()) if args.stdexec_source else "",
        "tools": {
            "cmake": version_of(["cmake", "--version"]),
            "ctest": version_of(["ctest", "--version"]),
            "ninja": version_of(["ninja", "--version"]),
            "wsl": version_of(["wsl.exe", "--status"]) if os.name == "nt" else None,
        },
    }


def configure_command(source: Path, build: Path, config: str, args: argparse.Namespace,
                      asan: bool, tsan: bool, ubsan: bool = False) -> list[str]:
    command = ["cmake", "-S", str(source), "-B", str(build), f"-DCMAKE_BUILD_TYPE={config}",
               f"-DC10_STUDY_BUILD_REFERENCE={'ON' if args.reference else 'OFF'}",
               f"-DC10_STUDY_TEST_STUDENTS={'ON' if args.students else 'OFF'}",
               f"-DC10_STUDY_ENABLE_ASAN={'ON' if asan else 'OFF'}",
               f"-DC10_STUDY_ENABLE_UBSAN={'ON' if ubsan else 'OFF'}",
               f"-DC10_STUDY_ENABLE_TSAN={'ON' if tsan else 'OFF'}"]
    if args.stdexec_source:
        command.append(f"-DFETCHCONTENT_SOURCE_DIR_STDEXEC={args.stdexec_source}")
    if args.cxx_standard:
        command.append(f"-DC10_CXX_STANDARD={args.cxx_standard}")
    if args.units:
        command.append(f"-DC10_UNITS={';'.join(args.units)}")
    if args.compiler:
        command.append(f"-DCMAKE_CXX_COMPILER={args.compiler}")
    if args.generator:
        command += ["-G", args.generator]
    if args.io_uring:
        command.append("-DC10_STUDY_ENABLE_IO_URING=ON")
    if args.liburing_root:
        command.append(f"-DC10_LIBURING_ROOT={args.liburing_root}")
    return command


def ensure_file_api_query(build: Path) -> None:
    query = build / ".cmake/api/v1/query"
    query.mkdir(parents=True, exist_ok=True)
    (query / "codemodel-v2").write_text("", encoding="utf-8")


def run_step(records: Path, name: str, command: list[str], timeout: float) -> dict[str, Any]:
    result = run_process(command, timeout)
    write_json(records / f"{name}.json", result)
    (records / f"{name}.stdout.txt").write_text(result.get("stdout", ""), encoding="utf-8")
    (records / f"{name}.stderr.txt").write_text(result.get("stderr", ""), encoding="utf-8")
    (records / f"{name}.argv.json").write_text(json.dumps(command, ensure_ascii=False, indent=2) + "\n",
                                                encoding="utf-8")
    return result


def show_only_tests(records: Path, build: Path, config: str, timeout: float) -> dict[str, Any]:
    result = run_step(records, "ctest-show-only", ["ctest", "--test-dir", str(build), "-C", config,
                                                  "--show-only=json-v1"], timeout)
    tests = []
    if result.get("status") == "PASS":
        try:
            data = json.loads(result.get("stdout", "{}"))
            tests = [str(test.get("name", "")) for test in data.get("tests", []) if test.get("name")]
            write_json(records / "ctest-show-only-parsed.json", data)
        except json.JSONDecodeError as error:
            result["status"] = "FAIL"
            result["error"] = f"show-only JSON parse failed: {error}"
    return {"result": result, "tests": tests}


def expected_tests(manifest: dict[str, Any], include_students: bool, include_reference: bool) -> list[str]:
    expected: list[str] = []
    for unit in manifest.get("units", []):
        if unit.get("kind") == "observation":
            name = unit.get("name")
            if name:
                expected.append(name)
            continue
        for key in (("reference_target", "good_target") if include_reference else ()):
            value = unit.get(key)
            if value:
                expected.append(value)
        bad = unit.get("bad_target")
        if bad and include_reference:
            expected.append(f"{bad}_rejected")
        if include_students and unit.get("student_target"):
            expected.append(unit["student_target"])
    return expected


def copy_test_records(build: Path, config: str, records: Path) -> dict[str, Any]:
    source = build / "records" / config
    dest = records / "test-records"
    copied = []
    counts: dict[str, int] = {}
    failures = []
    if not source.exists():
        return {"source": str(source), "dest": str(dest), "copied": 0, "counts": counts,
                "failures": [f"missing run_test records directory: {source}"]}
    dest.mkdir(parents=True, exist_ok=True)
    for path in sorted(source.glob("*.json")):
        target = dest / path.name
        shutil.copy2(path, target)
        copied.append(str(target))
        try:
            data = read_json(target)
            verdict = str(data.get("verdict", "UNKNOWN"))
        except json.JSONDecodeError:
            verdict = "MALFORMED"
        counts[verdict] = counts.get(verdict, 0) + 1
        if verdict in {"FAIL", "UNKNOWN", "MALFORMED"}:
            failures.append(str(target))
    return {"source": str(source), "dest": str(dest), "copied": len(copied), "files": copied,
            "counts": counts, "failures": failures}


def junit_counts(path: Path) -> dict[str, int]:
    if not path.exists():
        return {"tests": 0, "failures": 0, "skipped": 0}
    root = ET.parse(path).getroot()
    tests = int(root.attrib.get("tests", 0))
    failures = int(root.attrib.get("failures", 0)) + int(root.attrib.get("errors", 0))
    skipped = int(root.attrib.get("skipped", 0))
    return {"tests": tests, "failures": failures, "skipped": skipped}


def run_lane(args: argparse.Namespace, root: Path, config: str, sanitizer: str) -> dict[str, Any]:
    asan, tsan = sanitizer == "asan", sanitizer == "tsan"
    lane = f"{config}-{sanitizer or 'plain'}"
    records = root / lane
    build = args.build_root / root.name / lane
    ensure_file_api_query(build)
    configure = run_step(records, "configure", configure_command(args.source.resolve(), build, config, args, asan, tsan, sanitizer == "ubsan"),
                         args.configure_timeout)
    manifest = load_manifest(build)
    write_json(records / "unit-manifest.json", manifest)
    for directory in ("F01_native", "V1_nvexec"):
        for probe in (build / directory).rglob("*.txt"):
            destination = records / "capability-records" / directory / probe.relative_to(build / directory)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(probe, destination)
    build_result = run_step(records, "build", ["cmake", "--build", str(build), "--config", config,
                                                "--parallel", str(args.jobs)],
                            args.build_timeout) if configure["status"] == "PASS" else {}
    show = show_only_tests(records, build, config, args.test_timeout) if build_result.get("status") == "PASS" else {"tests": [], "result": {}}
    junit = (records / "ctest-junit.xml").resolve()
    ctest_command = ["ctest", "--test-dir", str(build), "-C", config, "--output-on-failure", "--output-junit", str(junit)]
    ctest = run_step(records, "ctest", ctest_command, args.test_timeout) if show.get("result", {}).get("status") == "PASS" else {}
    copied = copy_test_records(build, config, records) if ctest else {}
    registered = set(show.get("tests", []))
    discovered = show.get("result", {}).get("status") == "PASS"
    expected = expected_tests(manifest, args.students, args.reference)
    missing = sorted(test for test in expected if test not in registered) if discovered else []
    test_summary = {"discovered": discovered, "registered_count": len(registered), "registered": sorted(registered),
                    "expected": expected, "missing_expected": missing,
                    "junit": str(junit), "junit_counts": junit_counts(junit),
                    "records": copied}
    lane_failures = []
    if manifest.get("units") == []:
        lane_failures.append("unit-manifest is empty or missing")
    if build_result.get("status") == "PASS" and not discovered:
        lane_failures.append("ctest discovery failed")
    if discovered and not registered:
        lane_failures.append("ctest registered zero tests")
    if missing:
        lane_failures.append("missing expected tests: " + ", ".join(missing))
    if copied and copied.get("failures"):
        lane_failures.append("run_test records contain failures")
    return {"lane": lane, "config": config, "sanitizer": sanitizer or "none", "build_dir": str(build),
            "records": str(records), "configure": configure, "build": build_result, "ctest": ctest,
            "ctest_show_only": show, "test_summary": test_summary, "lane_failures": lane_failures,
            "cache": cache_values(build)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=EXERCISES)
    parser.add_argument("--records", type=Path, default=COURSE / "references/validation/matrix")
    parser.add_argument("--build-root", type=Path, default=REPO / "build/c10-matrix")
    parser.add_argument("--stdexec-source", type=Path)
    parser.add_argument("--config", action="append", choices=["Debug", "Release"], default=[])
    parser.add_argument("--sanitizer", action="append", choices=["asan", "ubsan", "tsan"], default=[],
                        help="Run only these sanitizer lanes; omit for plain builds")
    parser.add_argument("--unit", dest="units", action="append", default=[])
    parser.add_argument("--cxx-standard")
    parser.add_argument("--compiler")
    parser.add_argument("--generator")
    parser.add_argument("--io-uring", action="store_true")
    parser.add_argument("--liburing-root", type=Path)
    parser.add_argument("--reference", action="store_true", default=True)
    parser.add_argument("--no-reference", dest="reference", action="store_false")
    parser.add_argument("--students", action="store_true")
    parser.add_argument("--configure-timeout", type=float, default=180)
    parser.add_argument("--build-timeout", type=float, default=600)
    parser.add_argument("--jobs", type=int, default=1 if os.name == "nt" else 2)
    parser.add_argument("--test-timeout", type=float, default=300)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8]
    root = args.records / run_id
    configs = args.config or ["Debug", "Release"]
    sanitizers = args.sanitizer or [""]
    summary = {"run_id": run_id, "environment": env_record(args), "lanes": []}
    failures = []
    for config in configs:
        for sanitizer in sanitizers:
            lane = run_lane(args, root, config, sanitizer)
            summary["lanes"].append(lane)
            for step in ("configure", "build", "ctest"):
                data = lane.get(step) or {}
                if data.get("status") not in ("PASS", None):
                    failures.append(f"{lane['lane']} {step} failed")
            failures.extend(f"{lane['lane']} {failure}" for failure in lane.get("lane_failures", []))
    summary["verdict"] = "FAIL" if failures else "PASS"
    summary["failures"] = failures
    write_json(root / "summary.json", summary)
    print(f"{summary['verdict']}: {root}")
    for failure in failures:
        print(failure)
    return int(summary["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
