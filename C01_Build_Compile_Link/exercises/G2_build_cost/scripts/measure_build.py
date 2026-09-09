from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import shutil
import statistics
import sys
import tempfile
import time
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from process_runner import run_process  # noqa: E402

VARIANTS = ("baseline", "pch", "lto")
SCENARIOS = ("clean", "noop", "implementation", "public-header")
TARGETS = {
    "baseline": "G2_build_cost_baseline",
    "pch": "G2_build_cost_pch",
    "lto": "G2_build_cost_lto",
}
SOURCE_EXTS = {".cpp", ".hpp", ".h", ".cmake", ".txt", ".md", ".py", ".json"}


def digest_file(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def digest_tree(root: Path) -> str:
    result = hashlib.sha256()
    for directory, directories, files in os.walk(root):
        directories[:] = sorted(d for d in directories if d not in {"build", "out", "__pycache__"} and not d.startswith("."))
        for name in sorted(files):
            path = Path(directory) / name
            if path.suffix not in SOURCE_EXTS:
                continue
            result.update(path.relative_to(root).as_posix().encode("utf-8"))
            result.update(bytes.fromhex(digest_file(path)))
    return result.hexdigest()


def write_process(output_dir: Path, name: str, result: dict[str, Any]) -> None:
    (output_dir / f"{name}.stdout.txt").write_text(result["stdout"], encoding="utf-8", errors="replace")
    (output_dir / f"{name}.stderr.txt").write_text(result["stderr"], encoding="utf-8", errors="replace")
    meta = {k: v for k, v in result.items() if k not in {"stdout", "stderr"}}
    (output_dir / f"{name}.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False), encoding="utf-8")


def ensure_new_output(path: Path) -> None:
    if path.exists():
        raise FileExistsError(f"output directory already exists: {path}")
    path.mkdir(parents=True)


def copy_fixture(repo: Path, destination: Path) -> Path:
    exercises_src = repo / "C01_Build_Compile_Link" / "exercises"
    exercises_dst = destination / "C01_Build_Compile_Link" / "exercises"
    shutil.copytree(exercises_src / "G2_build_cost", exercises_dst / "G2_build_cost")
    shutil.copytree(exercises_src / "cmake", exercises_dst / "cmake")
    shutil.copytree(exercises_src / "include", exercises_dst / "include")
    return exercises_dst / "G2_build_cost"


def append_marker(source_root: Path, scenario: str, marker: str) -> str:
    if scenario == "implementation":
        path = source_root / "reference" / "common.cpp"
        text = f"\n// g2 implementation measurement marker: {marker}\n"
    elif scenario == "public-header":
        path = source_root / "reference" / "include" / "g2_common" / "heavy.hpp"
        text = f"\n// g2 public header measurement marker: {marker}\n"
    else:
        return ""
    with path.open("a", encoding="utf-8") as stream:
        stream.write(text)
    return path.relative_to(source_root).as_posix()


def parse_build_actions(stdout: str) -> dict[str, Any]:
    compiled: list[str] = []
    compile_lines = []
    for line in stdout.splitlines():
        is_compile = "cl.exe" in line and " -c " in line
        if not is_compile:
            continue
        compile_lines.append(line)
        for name in ("alpha.cpp", "beta.cpp", "gamma.cpp", "common.cpp", "main.cpp"):
            if name in line:
                compiled.append(name)
    return {
        "compiled_sources_seen": sorted(set(compiled)),
        "compile_line_count": len(compile_lines),
        "link_seen": "link.exe" in stdout or " vs_link_exe " in stdout,
        "ninja_no_work": "ninja: no work to do" in stdout,
        "dyndep_seen": "cmake_ninja_dyndep" in stdout,
    }


def run_checked(command: list[str], timeout: float, raw: Path, name: str) -> dict[str, Any]:
    result = run_process(command, timeout)
    write_process(raw, name, result)
    return result


def require_pass(result: dict[str, Any], what: str) -> None:
    if result["status"] != "PASS":
        raise RuntimeError(f"{what} failed: exit={result['exit_code']} timeout={result['timeout']} error={result['error']}")


def configure_command(cmake: str, ninja: str, source: Path, build: Path) -> list[str]:
    return [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja",
            f"-DCMAKE_MAKE_PROGRAM={ninja}", "-DCMAKE_BUILD_TYPE=Release"]


def build_command(cmake: str, build: Path, target: str, parallel: int) -> list[str]:
    return [cmake, "--build", str(build), "--target", target, "--parallel", str(parallel), "--verbose"]


def run_command(exe: Path) -> list[str]:
    return [str(exe)]


def run_round(args: argparse.Namespace, repo: Path, work_root: Path, raw: Path, item: dict[str, Any]) -> dict[str, Any]:
    variant = item["variant"]
    scenario = item["scenario"]
    phase = item["phase"]
    repetition = item["repetition"]
    target = TARGETS[variant]
    round_name = f"{phase}-{repetition:02d}-{variant}-{scenario}"
    round_dir = work_root / round_name
    source_root = copy_fixture(repo, round_dir / "source")
    build_dir = round_dir / "build"
    raw_prefix = round_name
    marker = f"seed={args.seed};phase={phase};rep={repetition};variant={variant};scenario={scenario}"
    mutated = ""
    timed_commands: list[dict[str, Any]] = []
    prepare_commands: list[dict[str, Any]] = []
    status = "PASS"
    failure = ""

    try:
        if scenario == "clean":
            configure = run_checked(configure_command(args.cmake, args.ninja, source_root, build_dir), args.timeout, raw, f"{raw_prefix}-timed-configure")
            timed_commands.append({"name": "configure", **{k: configure[k] for k in ("command", "exit_code", "timeout", "process_seconds", "status", "error", "cleanup_error")}})
            require_pass(configure, "clean configure")
            build = run_checked(build_command(args.cmake, build_dir, target, args.parallel), args.timeout, raw, f"{raw_prefix}-timed-build")
            timed_commands.append({"name": "build", "actions": parse_build_actions(build["stdout"]), **{k: build[k] for k in ("command", "exit_code", "timeout", "process_seconds", "status", "error", "cleanup_error")}})
            require_pass(build, "clean build")
            elapsed = sum(command["process_seconds"] for command in timed_commands)
        else:
            configure = run_checked(configure_command(args.cmake, args.ninja, source_root, build_dir), args.timeout, raw, f"{raw_prefix}-prepare-configure")
            prepare_commands.append({"name": "configure", **{k: configure[k] for k in ("command", "exit_code", "timeout", "process_seconds", "status", "error", "cleanup_error")}})
            require_pass(configure, "prepare configure")
            build0 = run_checked(build_command(args.cmake, build_dir, target, args.parallel), args.timeout, raw, f"{raw_prefix}-prepare-build")
            prepare_commands.append({"name": "build", "actions": parse_build_actions(build0["stdout"]), **{k: build0[k] for k in ("command", "exit_code", "timeout", "process_seconds", "status", "error", "cleanup_error")}})
            require_pass(build0, "prepare build")
            if scenario in {"implementation", "public-header"}:
                mutated = append_marker(source_root, scenario, marker)
            build = run_checked(build_command(args.cmake, build_dir, target, args.parallel), args.timeout, raw, f"{raw_prefix}-timed-build")
            timed_commands.append({"name": "build", "actions": parse_build_actions(build["stdout"]), **{k: build[k] for k in ("command", "exit_code", "timeout", "process_seconds", "status", "error", "cleanup_error")}})
            require_pass(build, f"{scenario} build")
            elapsed = sum(command["process_seconds"] for command in timed_commands)

        exe = build_dir / f"{target}.exe" if os.name == "nt" else build_dir / target
        check = run_checked(run_command(exe), args.timeout, raw, f"{raw_prefix}-check-run")
        require_pass(check, "semantic check")
        exe_sha = digest_file(exe)
    except BaseException as error:
        status = "FAIL"
        failure = f"{type(error).__name__}: {error}"
        elapsed = None
        exe_sha = ""

    source_sha = digest_tree(source_root) if source_root.exists() else ""
    return {
        "phase": phase,
        "repetition": repetition,
        "variant": variant,
        "target": target,
        "scenario": scenario,
        "status": status,
        "failure": failure,
        "elapsed_seconds": elapsed,
        "prepare_commands": prepare_commands,
        "timed_commands": timed_commands,
        "mutated_file": mutated,
        "source_sha256": source_sha,
        "exe_sha256": exe_sha,
        "round_dir": str(round_dir),
    }


def summarize(rounds: list[dict[str, Any]], samples: int, variants: tuple[str, ...], scenarios: tuple[str, ...]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    for variant in variants:
        for scenario in scenarios:
            formal = [r for r in rounds if r["phase"] == "sample" and r["variant"] == variant and r["scenario"] == scenario]
            valid = [r for r in formal if r["status"] == "PASS" and math.isfinite(r["elapsed_seconds"])]
            values = [r["elapsed_seconds"] * 1000.0 for r in valid]
            entry: dict[str, Any] = {"variant": variant, "scenario": scenario,
                                     "valid_samples": len(valid), "requested_samples": samples,
                                     "samples_ms": values}
            if len(valid) == samples:
                entry.update({"median_ms": statistics.median(values), "min_ms": min(values), "max_ms": max(values), "range_ms": max(values) - min(values)})
            else:
                entry.update({"status": "INVALID", "reason": "not all formal samples passed"})
            result.append(entry)
    return result


def self_test() -> int:
    scratch_parent = Path.cwd() / "C01_Build_Compile_Link" / "exercises" / "build"
    scratch_parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="g2-driver-selftest-", dir=scratch_parent) as temp:
        existing = Path(temp) / "existing"
        existing.mkdir()
        rejected_existing = False
        try:
            ensure_new_output(existing)
        except FileExistsError:
            rejected_existing = True
        fail = run_process([sys.executable, "-c", "import sys; sys.exit(3)"], 10)
        timeout = run_process([sys.executable, "-c", "import time; time.sleep(10)"], 0.2)
        ok = rejected_existing and fail["status"] == "FAIL" and fail["exit_code"] == 3 and timeout["status"] == "FAIL" and timeout["timeout"]
        print(json.dumps({"rejected_existing_output": rejected_existing, "failed_command_status": fail["status"],
                          "failed_command_exit": fail["exit_code"], "timeout_status": timeout["status"],
                          "timeout_seen": timeout["timeout"], "passed": ok}, indent=2))
        return 0 if ok else 1


def main() -> int:
    parser = argparse.ArgumentParser(description="Measure G2 build-cost scenarios with independent source/build copies.")
    parser.add_argument("--output", type=Path, required=False)
    parser.add_argument("--work-root", type=Path, help="new directory for per-round source/build copies; defaults to OUTPUT/rounds")
    parser.add_argument("--samples", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--parallel", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--ninja", default=shutil.which("ninja") or "ninja")
    parser.add_argument("--variant", action="append", choices=VARIANTS)
    parser.add_argument("--scenario", action="append", choices=SCENARIOS)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if args.output is None:
        parser.error("--output is required unless --self-test is used")
    if args.samples < 1 or args.warmups < 1 or args.parallel < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("samples, warmups, parallel and timeout must be positive")

    repo = Path(__file__).resolve().parents[4]
    output = args.output.resolve()
    ensure_new_output(output)
    raw = output / "raw"
    raw.mkdir()
    work_root = (args.work_root.resolve() if args.work_root else output / "rounds")
    ensure_new_output(work_root)

    variants = tuple(args.variant or VARIANTS)
    scenarios = tuple(args.scenario or SCENARIOS)
    rng = random.Random(args.seed)
    schedule = []
    for phase, count in (("warmup", args.warmups), ("sample", args.samples)):
        for repetition in range(count):
            items = [{"phase": phase, "repetition": repetition, "variant": variant, "scenario": scenario}
                     for variant in variants for scenario in scenarios]
            rng.shuffle(items)
            schedule.extend(items)

    report: dict[str, Any] = {
        "schema": 1,
        "status": "RUNNING",
        "repo": str(repo),
        "script": str(Path(__file__).resolve()),
        "environment": {"python": sys.version, "platform": platform.platform(), "machine": platform.machine(),
                         "logical_cpus": os.cpu_count()},
        "arguments": {"samples": args.samples, "warmups": args.warmups, "seed": args.seed,
                       "parallel": args.parallel, "timeout": args.timeout, "cmake": args.cmake,
                       "ninja": args.ninja, "variants": variants, "scenarios": scenarios, "work_root": str(work_root)},
        "fixture_source_sha256": digest_tree(repo / "C01_Build_Compile_Link" / "exercises" / "G2_build_cost"),
        "process_runner_sha256": digest_file(repo / "C01_Build_Compile_Link" / "exercises" / "tools" / "process_runner.py"),
        "schedule": schedule,
        "rounds": [],
        "summary": [],
    }
    (output / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    failures = 0
    for item in schedule:
        round_result = run_round(args, repo, work_root, raw, item)
        report["rounds"].append(round_result)
        if round_result["status"] != "PASS":
            failures += 1
        report["summary"] = summarize(report["rounds"], args.samples, variants, scenarios)
        report["status"] = "RUNNING_WITH_FAILURES" if failures else "RUNNING"
        (output / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    report["summary"] = summarize(report["rounds"], args.samples, variants, scenarios)
    invalid = [entry for entry in report["summary"] if entry.get("valid_samples") != args.samples]
    report["status"] = "FAIL" if failures or invalid else "PASS"
    (output / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps({"status": report["status"], "output": str(output), "summary": report["summary"]}, indent=2, ensure_ascii=False))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())






