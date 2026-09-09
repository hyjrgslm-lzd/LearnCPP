from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "Engineering_Study" / "exercises" / "tools"))
from process_runner import run_process  # noqa: E402


def write_step(out: Path, name: str, result: dict[str, Any]) -> None:
    (out / f"{name}.stdout.txt").write_text(result["stdout"], encoding="utf-8", errors="replace")
    (out / f"{name}.stderr.txt").write_text(result["stderr"], encoding="utf-8", errors="replace")
    meta = {k: v for k, v in result.items() if k not in {"stdout", "stderr"}}
    (out / f"{name}.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False), encoding="utf-8")


def run_step(out: Path, name: str, command: list[str], timeout: float) -> dict[str, Any]:
    result = run_process(command, timeout)
    write_step(out, name, result)
    return result


def ok(result: dict[str, Any]) -> bool:
    return result["status"] == "PASS" and result["exit_code"] == 0 and not result["timeout"] and not result["cleanup_error"] and not result["error"]


def expected_check_failure(result: dict[str, Any]) -> bool:
    text = result["stdout"] + result["stderr"]
    return (
        result["exit_code"] not in (0, None)
        and not result["timeout"]
        and not result["cleanup_error"]
        and not result["error"]
        and "check failed:" in text
    )


def copy_case(exercise: str, case_root: Path) -> Path:
    src = ROOT / "Engineering_Study" / "exercises"
    dst = case_root / "Engineering_Study" / "exercises"
    shutil.copytree(src / "cmake", dst / "cmake")
    shutil.copytree(src / "include", dst / "include")
    shutil.copytree(src / exercise, dst / exercise)
    return dst / exercise


def patch_g1(source: Path, kind: str) -> None:
    if kind == "good":
        text = r'''#include "parser.hpp"

#include <stdexcept>

int parse_two_digits(std::string_view text) {
    if (text.size() != 2 || text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9') {
        throw std::invalid_argument("expected exactly two ASCII digits");
    }
    return (text[0] - '0') * 10 + (text[1] - '0');
}
'''
    elif kind == "constant42":
        text = r'''#include "parser.hpp"

int parse_two_digits(std::string_view) {
    return 42;
}
'''
    else:
        raise ValueError(kind)
    (source / "src" / "student" / "parser.cpp").write_text(text, encoding="utf-8")


def patch_f2(source: Path, kind: str) -> None:
    if kind == "good":
        text = r'''#include "student.hpp"

#include <f2_provider/provider.hpp>

int student_use_dependency(int input) {
    return f2_provider::compute_answer(input);
}
'''
    elif kind == "formula":
        text = r'''#include "student.hpp"

#include <f2_provider/provider.hpp>

int student_use_dependency(int input) {
    (void)f2_provider::api_version;
    return input * 2 + 2;
}
'''
    else:
        raise ValueError(kind)
    (source / "src" / "student" / "student.cpp").write_text(text, encoding="utf-8")


def cmake_configure(cmake: str, ninja: str, source: Path, build: Path) -> list[str]:
    return [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM={ninja}", "-DCMAKE_BUILD_TYPE=Release", "-DENGINEERING_STUDY_TEST_STUDENTS=ON"]


def cmake_build(cmake: str, build: Path, *targets: str) -> list[str]:
    return [cmake, "--build", str(build), "--target", *targets, "--verbose"]


def ctest(build: Path, regex: str) -> list[str]:
    return ["ctest", "--test-dir", str(build), "-R", regex, "--output-on-failure"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--work-root", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("--cmake", default="D:/cmake/install/bin/cmake.exe")
    parser.add_argument("--ninja", default="D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
    args = parser.parse_args()

    out = args.output.resolve()
    work = args.work_root.resolve()
    if out.exists() or work.exists():
        parser.error("--output and --work-root must be new directories")
    out.mkdir(parents=True)
    work.mkdir(parents=True)

    report: dict[str, Any] = {"status": "RUNNING", "cases": []}

    def record(name: str, passed: bool, detail: str) -> None:
        report["cases"].append({"name": name, "passed": passed, "detail": detail})
        (out / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    # G1 good implementation: Reference and Student must both pass the same exhaustive contract.
    g1_good = copy_case("G1_diagnostics", work / "g1-good")
    patch_g1(g1_good, "good")
    g1_good_build = work / "g1-good-build"
    steps = [
        ("g1-good-configure", cmake_configure(args.cmake, args.ninja, g1_good, g1_good_build)),
        ("g1-good-build", cmake_build(args.cmake, g1_good_build, "G1_diagnostics_reference", "G1_diagnostics_student")),
        ("g1-good-ctest", ctest(g1_good_build, "G1_diagnostics_(reference|student)")),
    ]
    passed = True
    for step, command in steps:
        result = run_step(out, step, command, args.timeout)
        passed = passed and ok(result)
    record("g1-good-contract", passed, "good parser passes reference and student exhaustive contract")

    # G1 constant mutant: build must succeed, Student contract must fail.
    g1_bad = copy_case("G1_diagnostics", work / "g1-constant42")
    patch_g1(g1_bad, "constant42")
    g1_bad_build = work / "g1-constant42-build"
    r1 = run_step(out, "g1-constant42-configure", cmake_configure(args.cmake, args.ninja, g1_bad, g1_bad_build), args.timeout)
    r2 = run_step(out, "g1-constant42-build", cmake_build(args.cmake, g1_bad_build, "G1_diagnostics_student"), args.timeout)
    r3 = run_step(out, "g1-constant42-ctest", ctest(g1_bad_build, "G1_diagnostics_student"), args.timeout)
    record("g1-constant42-rejected", ok(r1) and ok(r2) and expected_check_failure(r3), "constant 42 parser builds but fails student contract")

    # F2 good implementation: real provider and spy provider student checks must pass.
    f2_good = copy_case("F2_dependencies", work / "f2-good")
    patch_f2(f2_good, "good")
    f2_good_build = work / "f2-good-build"
    steps = [
        ("f2-good-configure", cmake_configure(args.cmake, args.ninja, f2_good, f2_good_build)),
        ("f2-good-build", cmake_build(args.cmake, f2_good_build, "F2_dependencies_reference", "F2_dependencies_student", "F2_dependencies_student_delegation")),
        ("f2-good-ctest", ctest(f2_good_build, "F2_dependencies_(reference|student|student_delegation)")),
    ]
    passed = True
    for step, command in steps:
        result = run_step(out, step, command, args.timeout)
        passed = passed and ok(result)
    record("f2-good-delegation", passed, "good student passes real provider and spy provider checks")

    # F2 formula mutant: real provider check passes, spy delegation check must fail.
    f2_bad = copy_case("F2_dependencies", work / "f2-formula-mutant")
    patch_f2(f2_bad, "formula")
    f2_bad_build = work / "f2-formula-mutant-build"
    r1 = run_step(out, "f2-formula-configure", cmake_configure(args.cmake, args.ninja, f2_bad, f2_bad_build), args.timeout)
    r2 = run_step(out, "f2-formula-build", cmake_build(args.cmake, f2_bad_build, "F2_dependencies_student", "F2_dependencies_student_delegation"), args.timeout)
    r3 = run_step(out, "f2-formula-real-ctest", ctest(f2_bad_build, "^F2_dependencies_student$"), args.timeout)
    r4 = run_step(out, "f2-formula-spy-ctest", ctest(f2_bad_build, "F2_dependencies_student_delegation"), args.timeout)
    record("f2-formula-mutant-rejected", ok(r1) and ok(r2) and ok(r3) and expected_check_failure(r4), "formula mutant passes real provider values but fails spy delegation check")

    failures = [case for case in report["cases"] if not case["passed"]]
    report["status"] = "FAIL" if failures else "PASS"
    report["failures"] = failures
    (out / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
