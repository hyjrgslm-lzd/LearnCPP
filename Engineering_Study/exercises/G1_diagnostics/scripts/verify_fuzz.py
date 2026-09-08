from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import sys
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from process_runner import run_process  # noqa: E402

MARKER = "G1_ORACLE_MISMATCH:"


def write_step(output: Path, name: str, result: dict[str, Any]) -> None:
    (output / f"{name}.stdout.txt").write_text(result["stdout"], encoding="utf-8", errors="replace")
    (output / f"{name}.stderr.txt").write_text(result["stderr"], encoding="utf-8", errors="replace")
    meta = {k: v for k, v in result.items() if k not in {"stdout", "stderr"}}
    (output / f"{name}.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False), encoding="utf-8")


def step_ok(result: dict[str, Any]) -> bool:
    return result["status"] == "PASS" and result["exit_code"] == 0 and not result["timeout"] and not result["cleanup_error"] and not result["error"]


def bad_rejected_by_oracle(result: dict[str, Any]) -> bool:
    return (
        result["exit_code"] not in (0, None)
        and not result["timeout"]
        and not result["cleanup_error"]
        and not result["error"]
        and MARKER in result["stderr"]
    )


def copy_seed_corpus(source: Path, destination: Path) -> list[str]:
    if destination.exists():
        raise FileExistsError(f"corpus destination already exists: {destination}")
    destination.mkdir(parents=True)
    names = []
    for path in sorted((source / "fuzz_corpus").iterdir()):
        if path.is_file():
            shutil.copy2(path, destination / path.name)
            names.append(path.name)
    return names


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--clang", default=shutil.which("clang++"))
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    if not args.clang:
        parser.error("--clang is required when clang++ is not on PATH")
    out = args.output.resolve()
    if out.exists():
        parser.error(f"output already exists: {out}")
    out.mkdir(parents=True)
    source = args.source.resolve()
    build = out / "bin"
    build.mkdir()
    real_corpus = out / "input-corpus-real"
    bad_corpus = out / "input-corpus-bad"
    seeds = copy_seed_corpus(source, real_corpus)
    copy_seed_corpus(source, bad_corpus)
    artifact_dir = out / "artifacts"
    artifact_dir.mkdir()
    real = build / "g1_real_parser_fuzz.exe"
    bad = build / "g1_bad_parser_fuzz.exe"

    report: dict[str, Any] = {"status": "RUNNING", "steps": [], "seed_corpus": seeds, "marker": MARKER}

    commands = [
        ("compile-real", [args.clang, "-std=c++23", "-fsanitize=fuzzer", str(source / "fuzz_parse.cpp"), str(source / "reference" / "parser.cpp"), f"-I{source / 'reference'}", "-o", str(real)], "must_pass"),
        ("run-real", [str(real), "-runs=16", "-seed=20260908", "-max_total_time=5", f"-artifact_prefix={artifact_dir}{'/'}", str(real_corpus)], "must_pass"),
        ("compile-bad", [args.clang, "-std=c++23", "-fsanitize=fuzzer", str(source / "fuzz_parse.cpp"), str(source / "fuzz_bad" / "always_42_parser.cpp"), f"-I{source / 'reference'}", "-o", str(bad)], "must_pass"),
        ("run-bad", [str(bad), "-runs=16", "-seed=20260908", "-max_total_time=5", f"-artifact_prefix={artifact_dir}{'/'}", str(bad_corpus)], "must_fail_with_marker"),
    ]

    failures: list[str] = []
    for name, command, expectation in commands:
        result = run_process(command, args.timeout)
        write_step(out, name, result)
        if expectation == "must_pass":
            passed = step_ok(result)
        else:
            passed = bad_rejected_by_oracle(result)
        step = {"name": name, "expectation": expectation, "passed": passed, "exit_code": result["exit_code"], "timeout": result["timeout"], "status": result["status"]}
        report["steps"].append(step)
        if not passed:
            failures.append(name)
        (out / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    report["status"] = "PASS" if not failures else "FAIL"
    report["failures"] = failures
    (out / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
