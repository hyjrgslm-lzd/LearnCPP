"""Run checked C++ benchmark cases in fresh, externally bounded processes.

Example (run from exercises):
  python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe \
    --check build/bench/runtime_tests/Release/runtime_queue_history_test.exe \
    --variant mutex --variant ring --output build/results/queue-1 -- --size 100003
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path
import platform
import random
import signal
import statistics
import subprocess
import sys
import time


FIELDS = ("suite", "variant", "size", "threads", "milliseconds", "completed", "details")
CASE_FIELDS = ("suite", "variant", "size", "threads")


def digest_file(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def source_digest(root: Path) -> str:
    result = hashlib.sha256()
    for directory, directories, files in os.walk(root):
        directories[:] = sorted(d for d in directories if not (
            d.startswith("build") or d.startswith(".") or d in {"third_party", "__pycache__", "measurements"}))
        for name in sorted(files):
            path = Path(directory) / name
            if path.suffix not in {".cpp", ".hpp", ".h", ".cmake", ".txt", ".md", ".py", ".json"}:
                continue
            result.update(path.relative_to(root).as_posix().encode())
            result.update(bytes.fromhex(digest_file(path)))
    return result.hexdigest()


def run_process(command: list[str], timeout: float) -> dict:
    if not command or not math.isfinite(timeout) or timeout <= 0:
        raise ValueError("a command and a finite positive timeout are required before launch")
    settings = {"creationflags": subprocess.CREATE_NO_WINDOW | subprocess.CREATE_NEW_PROCESS_GROUP} if os.name == "nt" else {"start_new_session": True}
    started = time.monotonic()
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   text=True, encoding="utf-8", errors="replace", **settings)
    except OSError as error:
        return {"command": command, "exit_code": None, "timeout": False,
                "process_seconds": time.monotonic() - started, "stdout": "", "stderr": "",
                "cleanup_error": "", "error": f"launch failed: {error}", "status": "FAIL"}
    timed_out = False
    cleanup_errors = []
    failure = ""
    stdout = stderr = ""
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except BaseException as error:
        # Also cover interruption or pipe/wait failures after a child exists.
        timed_out = isinstance(error, subprocess.TimeoutExpired)
        failure = f"{type(error).__name__}: {error}"
        try:
            if os.name == "nt":
                killed = subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                                        capture_output=True, text=True, errors="replace", timeout=5,
                                        creationflags=subprocess.CREATE_NO_WINDOW)
                if killed.returncode:
                    cleanup_errors.append("tree termination: " + (killed.stderr or killed.stdout))
            else:
                os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        except BaseException as cleanup_error:
            cleanup_errors.append(f"tree termination failed: {cleanup_error}")
        finally:
            # The root handle is ours; a failure of taskkill must never bypass
            # direct termination. Failure to confirm descendants is reported.
            try:
                if process.poll() is None:
                    process.kill()
            except ProcessLookupError:
                pass
            except BaseException as cleanup_error:
                cleanup_errors.append(f"root termination failed: {cleanup_error}")
        try:
            stdout, stderr = process.communicate(timeout=5)
        except BaseException as cleanup_error:
            cleanup_errors.append(f"pipe collection failed: {cleanup_error}")
            try:
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5)
            except BaseException as reap_error:
                cleanup_errors.append(f"reap failed: {reap_error}")
    return {"command": command, "exit_code": process.returncode, "timeout": timed_out,
            "process_seconds": time.monotonic() - started,
            "stdout": stdout, "stderr": stderr, "cleanup_error": "; ".join(cleanup_errors), "error": failure,
            "status": "FAIL" if failure or cleanup_errors or process.returncode not in (0, 77) else
                      "SKIP" if process.returncode == 77 else "PASS"}


def parse_rows(output: str) -> list[dict]:
    try:
        reader = csv.DictReader(io.StringIO(output), strict=True)
        if reader.fieldnames != list(FIELDS):
            raise ValueError("benchmark stdout must contain the documented CSV header only")
        rows = list(reader)
    except csv.Error as error:
        raise ValueError(f"malformed CSV: {error}") from error
    if not rows:
        raise ValueError("successful benchmark emitted no measurements")
    for row in rows:
        if set(row) != set(FIELDS) or any(value is None for value in row.values()):
            raise ValueError("malformed CSV row")
        for field in ("size", "threads", "completed"):
            if not row[field].isascii() or not row[field].isdigit():
                raise ValueError(f"invalid {field}")
            row[field] = int(row[field])
        row["milliseconds"] = float(row["milliseconds"])
        if not math.isfinite(row["milliseconds"]) or row["milliseconds"] < 0:
            raise ValueError("invalid duration")
        if not row["suite"] or not row["variant"]:
            raise ValueError("suite and variant must be named")
    return rows


def unique_cases(rows: list[dict]) -> set[tuple]:
    cases = [tuple(row[field] for field in CASE_FIELDS) for row in rows]
    if len(set(cases)) != len(cases):
        raise ValueError("one process emitted the same case more than once")
    return set(cases)


def summarize(measurements: list[dict], samples: int) -> list[dict]:
    groups: dict[tuple, list[dict]] = {}
    for row in measurements:
        key = tuple(row[field] for field in CASE_FIELDS)
        groups.setdefault(key, []).append(row)
    summaries = []
    for key, rows in groups.items():
        if len(rows) != samples or {row["repetition"] for row in rows} != set(range(samples)):
            raise ValueError("each case requires exactly one result from each independent formal repetition")
        if len({row["completed"] for row in rows}) != 1:
            raise ValueError("fixed-work case changed its completed work between samples")
        values = [row["milliseconds"] for row in rows]
        summaries.append({"case": {**dict(zip(CASE_FIELDS, key)), "completed": rows[0]["completed"]},
            "sample_details": [row["details"] for row in rows], "samples_ms": values,
            "median_ms": statistics.median(values), "min_ms": min(values), "max_ms": max(values),
            "stdev_ms": statistics.stdev(values) if len(values) > 1 else 0})
    return summaries


def completion_code(status: str, allow_partial: bool = False) -> int:
    if status == "PASS" or (status == "PARTIAL_SKIP" and allow_partial):
        return 0
    return 77 if status in {"SKIP", "PARTIAL_SKIP"} else 1


def collect_environment(executable: Path) -> dict:
    environment = {"python": sys.version, "os": platform.platform(),
                   "machine": platform.machine(), "processor": platform.processor(),
                   "logical_cpus": os.cpu_count(), "executable_sha256": digest_file(executable)}
    for directory in executable.parents:
        cache = directory / "CMakeCache.txt"
        if cache.is_file():
            environment["cmake_cache"] = str(cache)
            wanted = ("CMAKE_BUILD_TYPE:", "CMAKE_CXX_FLAGS", "CMAKE_CXX_COMPILER:",
                      "CMAKE_GENERATOR:", "CMAKE_GENERATOR_INSTANCE:", "CONCURRENCY_STUDY_", "CS_HAS_")
            environment["cmake_settings"] = [line for line in cache.read_text(encoding="utf-8").splitlines()
                                              if line.startswith(wanted)]
            for compiler in sorted((directory / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake")):
                environment["compiler_configuration"] = str(compiler)
                environment["compiler_identity"] = [line for line in compiler.read_text(encoding="utf-8").splitlines()
                    if line.startswith(("set(CMAKE_CXX_COMPILER ", "set(CMAKE_CXX_COMPILER_ID ",
                                        "set(CMAKE_CXX_COMPILER_VERSION ", "set(CMAKE_CXX_PLATFORM_ID "))]
            break
    return environment


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--check", required=True, action="append", type=Path,
                        help="reference executable to run before measuring; repeat as needed")
    parser.add_argument("--variant", required=True, action="append")
    parser.add_argument("--samples", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--allow-partial", action="store_true", help="explicitly permit available samples when a requested variant is SKIP")
    parser.add_argument("benchmark_args", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.samples < 1 or args.warmups < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("samples, warmups and timeout must be positive")
    if "all" in args.variant or len(set(args.variant)) != len(args.variant):
        parser.error("select distinct concrete variants, not 'all'")
    extra = args.benchmark_args[1:] if args.benchmark_args[:1] == ["--"] else args.benchmark_args
    if "--variant" in extra:
        parser.error("--variant is controlled by the runner")
    executable = args.exe.resolve(strict=True)
    checks = [check.resolve(strict=True) for check in args.check]
    output = args.output.resolve()
    if output.exists():
        parser.error("output must be a new directory; existing measurements are never overwritten")
    output.mkdir(parents=True)
    course_root = Path(__file__).resolve().parents[2]
    snapshot = source_digest(course_root)
    report = {"schema": 1, "status": "RUNNING", "source_root": str(course_root),
              "source_sha256": snapshot, "environment": collect_environment(executable),
              "runner_command": sys.argv, "seed": args.seed,
              "allow_partial": args.allow_partial,
              "timing": "C++ reported interval; external process duration recorded separately",
              "threads_field": "positive: reported thread count; zero: implementation-managed count was not measured",
              "checks": [], "runs": [], "summaries": []}
    measurements = []
    try:
        for check in checks:
            result = run_process([str(check)], args.timeout)
            result["executable_sha256"] = digest_file(check)
            report["checks"].append(result)
            if result["status"] != "PASS":
                report["status"] = result["status"]
                return 77 if result["status"] == "SKIP" else 1
        rng = random.Random(args.seed)
        expected_cases = {}
        for repetition in range(args.warmups + args.samples):
            variants = args.variant.copy()
            rng.shuffle(variants)
            for variant in variants:
                result = run_process([str(executable), "--variant", variant, *extra], args.timeout)
                result.update(variant=variant, repetition=repetition, warmup=repetition < args.warmups)
                report["runs"].append(result)
                if result["status"] == "PASS":
                    try:
                        rows = parse_rows(result["stdout"])
                        if any(row["variant"] != variant for row in rows):
                            raise ValueError("the executable did not report the requested concrete variant")
                        cases = unique_cases(rows)
                        if variant in expected_cases and cases != expected_cases[variant]:
                            raise ValueError("case set changed between warmup and formal processes")
                        expected_cases[variant] = cases
                        result["rows"] = rows
                        if not result["warmup"]:
                            measurements.extend({**row, "repetition": repetition - args.warmups} for row in rows)
                    except ValueError as error:
                        result["status"] = "FAIL"
                        result["validation_error"] = str(error)
                if result["status"] == "FAIL":
                    report["status"] = "FAIL"
                    return 1
        for variant in args.variant:
            observed = {run["status"] for run in report["runs"] if run["variant"] == variant}
            if len(observed) != 1:
                raise ValueError(f"{variant}: capability changed between warmup and samples; PASS/SKIP cannot be mixed")
        report["summaries"] = summarize(measurements, args.samples)
        if source_digest(course_root) != snapshot or digest_file(executable) != report["environment"]["executable_sha256"]:
            raise ValueError("source or executable changed during measurement; results are not a stable snapshot")
        statuses = {run["status"] for run in report["runs"]}
        report["status"] = "PASS" if statuses == {"PASS"} else "PARTIAL_SKIP" if measurements else "SKIP"
        return completion_code(report["status"], args.allow_partial)
    except Exception as error:
        report["status"] = "FAIL"
        report["error"] = f"{type(error).__name__}: {error}"
        return 1
    finally:
        (output / "run.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        with (output / "samples.csv").open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=[*FIELDS, "repetition"])
            writer.writeheader()
            writer.writerows(measurements)
        print(f"{report['status']}: {output / 'run.json'}")


if __name__ == "__main__":
    raise SystemExit(main())
