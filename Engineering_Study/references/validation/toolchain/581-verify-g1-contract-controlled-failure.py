from __future__ import annotations
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
BASE = ROOT / "Engineering_Study" / "exercises"
RAW = ROOT / "Engineering_Study" / "references" / "validation" / "toolchain" / "g1-contract-controlled-failure-r1"
WORK = ROOT / "Engineering_Study" / "exercises" / "build" / "foundations-independent-review" / "g1-contract-controlled-failure-r1"
CMAKE = r"D:\cmake\install\bin\cmake.exe"
NINJA = r"D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

GOOD = r'''#include "parser.hpp"

#include <stdexcept>

int parse_two_digits(std::string_view text) {
    if (text.size() != 2 || text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9') {
        throw std::invalid_argument("expected exactly two ASCII digits");
    }
    return (text[0] - '0') * 10 + (text[1] - '0');
}
'''

CONSTANT42 = r'''#include "parser.hpp"

int parse_two_digits(std::string_view) {
    return 42;
}
'''

SPECIAL42 = r'''#include "parser.hpp"

#include <stdexcept>

int parse_two_digits(std::string_view text) {
    if (text == "42") {
        return 42;
    }
    throw std::invalid_argument("not 42");
}
'''

WRONG_EXCEPTION = r'''#include "parser.hpp"

#include <stdexcept>

int parse_two_digits(std::string_view text) {
    if (text.size() == 2 && text[0] >= '0' && text[0] <= '9' && text[1] >= '0' && text[1] <= '9') {
        return (text[0] - '0') * 10 + (text[1] - '0');
    }
    throw std::runtime_error("wrong exception type");
}
'''

CASES = {
    "good": {"source": GOOD, "expect": "pass"},
    "constant42": {"source": CONSTANT42, "expect": "controlled_fail", "needle": "check failed: parser must accept every two-digit ASCII input from 00 to 99"},
    "special42": {"source": SPECIAL42, "expect": "controlled_fail", "needle": "check failed: parser must not throw for valid two-digit ASCII input"},
    "wrong_exception": {"source": WRONG_EXCEPTION, "expect": "controlled_fail", "needle": "check failed: parser must reject invalid input with invalid_argument, not another exception"},
}
CONFIGS = ["Debug", "Release"]


def reset_dirs() -> None:
    if RAW.exists():
        shutil.rmtree(RAW)
    if WORK.exists():
        shutil.rmtree(WORK)
    RAW.mkdir(parents=True)
    WORK.mkdir(parents=True)


def decode(data: bytes | None) -> str:
    return (data or b"").decode("utf-8", errors="replace")


def run(name: str, command: list[str], timeout: float = 120.0) -> tuple[dict, str]:
    started = time.time()
    try:
        p = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=False, timeout=timeout)
        stdout = decode(p.stdout)
        stderr = decode(p.stderr)
        meta = {"name": name, "command": command, "exit_code": p.returncode, "timeout": False, "elapsed_seconds": time.time() - started}
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout if isinstance(exc.stdout, str) else decode(exc.stdout)
        stderr = exc.stderr if isinstance(exc.stderr, str) else decode(exc.stderr)
        meta = {"name": name, "command": command, "exit_code": None, "timeout": True, "elapsed_seconds": time.time() - started}
    (RAW / f"{name}.stdout.txt").write_text(stdout, encoding="utf-8", errors="replace")
    (RAW / f"{name}.stderr.txt").write_text(stderr, encoding="utf-8", errors="replace")
    (RAW / f"{name}.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False), encoding="utf-8")
    return meta, stdout + stderr


def ok(meta: dict) -> bool:
    return meta["exit_code"] == 0 and not meta["timeout"]


def controlled_fail(meta: dict, output: str, needle: str) -> bool:
    if meta["timeout"] or meta["exit_code"] in (0, None):
        return False
    if "0xc0000409" in output or "Exception:" in output or "terminate called" in output:
        return False
    return needle in output


def copy_case(case_name: str, source_text: str) -> Path:
    dstbase = WORK / case_name / "Engineering_Study" / "exercises"
    shutil.copytree(BASE / "cmake", dstbase / "cmake")
    shutil.copytree(BASE / "include", dstbase / "include")
    shutil.copytree(BASE / "G1_diagnostics", dstbase / "G1_diagnostics")
    exercise = dstbase / "G1_diagnostics"
    (exercise / "src" / "student" / "parser.cpp").write_text(source_text, encoding="utf-8")
    return exercise


def configure_cmd(src: Path, build: Path, config: str) -> list[str]:
    return [CMAKE, "-S", str(src), "-B", str(build), "-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM={NINJA}", f"-DCMAKE_BUILD_TYPE={config}", "-DENGINEERING_STUDY_TEST_STUDENTS=ON"]


def build_cmd(build: Path) -> list[str]:
    return [CMAKE, "--build", str(build), "--target", "G1_diagnostics_student", "--verbose"]


def ctest_cmd(build: Path) -> list[str]:
    return ["ctest", "--test-dir", str(build), "-R", "^G1_diagnostics_student$", "--output-on-failure"]


def main() -> int:
    reset_dirs()
    results = []
    for config in CONFIGS:
        for case_name, case in CASES.items():
            src = copy_case(f"{config}-{case_name}", case["source"])
            build = WORK / f"{config}-{case_name}-build"
            prefix = f"{config.lower()}-{case_name}"
            r_cfg, out_cfg = run(f"{prefix}-configure", configure_cmd(src, build, config))
            r_build, out_build = run(f"{prefix}-build", build_cmd(build))
            r_test, out_test = run(f"{prefix}-ctest", ctest_cmd(build))
            if case["expect"] == "pass":
                passed = ok(r_cfg) and ok(r_build) and ok(r_test)
            else:
                passed = ok(r_cfg) and ok(r_build) and controlled_fail(r_test, out_test, case["needle"])
            results.append({
                "config": config,
                "case": case_name,
                "passed": passed,
                "configure_exit": r_cfg["exit_code"],
                "build_exit": r_build["exit_code"],
                "ctest_exit": r_test["exit_code"],
                "ctest_timeout": r_test["timeout"],
                "expect": case["expect"],
            })
    report = {"status": "PASS" if all(r["passed"] for r in results) else "FAIL", "results": results}
    (RAW / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0 if report["status"] == "PASS" else 1

if __name__ == "__main__":
    raise SystemExit(main())
