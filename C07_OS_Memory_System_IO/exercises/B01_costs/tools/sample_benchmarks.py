"""Run B01 benchmark processes with raw evidence and strict sample accounting."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import random
import statistics
import sys
import tempfile
from typing import Any

TOOLS = Path(__file__).resolve().parents[2] / "tools"
sys.path.insert(0, str(TOOLS))
from run_test import supervise  # noqa: E402


SOURCE_FILES = [
    "C07_OS_Memory_System_IO/exercises/B01_costs/benchmark.cpp",
    "C07_OS_Memory_System_IO/exercises/B01_costs/CMakeLists.txt",
    "C07_OS_Memory_System_IO/exercises/B01_costs/tools/sample_benchmarks.py",
    "C07_OS_Memory_System_IO/exercises/include/c07/file_pipeline_types.hpp",
    "C07_OS_Memory_System_IO/exercises/include/c07/file_pipeline_io.hpp",
    "C07_OS_Memory_System_IO/exercises/include/c07/os.hpp",
    "C07_OS_Memory_System_IO/exercises/include/c07/memory.hpp",
    "C07_OS_Memory_System_IO/exercises/include/c07/completion_io.hpp",
    "C07_OS_Memory_System_IO/exercises/P1_file_pipeline/CMakeLists.txt",
    "C07_OS_Memory_System_IO/exercises/P1_file_pipeline/checks/native_failure_checks.cpp",
    "C07_OS_Memory_System_IO/exercises/P1_file_pipeline/src/reference/pipeline.hpp",
    "C07_OS_Memory_System_IO/exercises/L05_pmr/src/reference/pmr_lab.hpp",
    "C07_OS_Memory_System_IO/exercises/cmake/StudySetup.cmake",
]

IO_SIZES = [4 * 1024, 1024 * 1024, 8 * 1024 * 1024]
ALLOC_ITEMS = [4096, 32768]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def fingerprint(repo_root: Path, exe: Path) -> dict[str, Any]:
    sources: dict[str, str] = {}
    missing: list[str] = []
    for name in SOURCE_FILES:
        path = repo_root / name
        if path.exists():
            sources[name] = sha256(path)
        else:
            missing.append(name)
    return {
        "exe": str(exe),
        "exe_sha256": sha256(exe),
        "sources": sources,
        "missing_sources": missing,
    }


def cases(phase: str) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    if phase == "baseline":
        result.extend({"kind": "io", "variant": "buffered", "bytes": size, "rounds": 2, "depth": 4}
                      for size in IO_SIZES)
        result.extend({"kind": "alloc", "variant": "heap", "items": items} for items in ALLOC_ITEMS)
    elif phase == "compare":
        result.extend({"kind": "io", "variant": "mapped", "bytes": size, "rounds": 2, "depth": 4}
                      for size in IO_SIZES)
        for depth in (1, 4):
            result.extend({"kind": "io", "variant": "completion", "bytes": size, "rounds": 2, "depth": depth}
                          for size in IO_SIZES)
        for variant in ("arena", "pool", "std-monotonic", "std-pool"):
            result.extend({"kind": "alloc", "variant": variant, "items": items} for items in ALLOC_ITEMS)
    else:
        raise ValueError(f"unknown phase: {phase}")
    return result


def case_id(case: dict[str, Any]) -> str:
    if case["kind"] == "alloc":
        return f"alloc-{case['variant']}-{case['items']}"
    return f"io-{case['variant']}-{case['bytes']}-r{case['rounds']}-d{case['depth']}"


def command(exe: Path, case: dict[str, Any]) -> list[str]:
    if case["kind"] == "alloc":
        return [str(exe), "alloc", case["variant"], str(case["items"])]
    return [str(exe), "io", case["variant"], str(case["bytes"]), str(case["rounds"]), str(case["depth"])]


def parse_json_stdout(stdout: str) -> tuple[dict[str, Any] | None, str]:
    lines = [line for line in stdout.splitlines() if line.strip()]
    json_lines = [line for line in lines if line.lstrip().startswith("{")]
    if len(json_lines) != 1:
        return None, f"expected exactly one JSON stdout line, got {len(json_lines)}"
    try:
        parsed = json.loads(json_lines[0])
    except json.JSONDecodeError as error:
        return None, f"invalid JSON stdout: {error}"
    if not isinstance(parsed, dict):
        return None, "JSON stdout is not an object"
    return parsed, ""


def finite_nonnegative(value: Any, *, positive: bool = False) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) and (
        value > 0 if positive else value >= 0)


def integer_value(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool) and value >= 0


def validate_parsed(parsed: dict[str, Any], case: dict[str, Any]) -> str:
    if parsed.get("valid") is not True:
        return "JSON result is not marked valid"
    if parsed.get("kind") != case["kind"] or parsed.get("variant") != case["variant"]:
        return "JSON kind or variant does not match case"
    if not finite_nonnegative(parsed.get("seconds"), positive=True):
        return "seconds must be finite, non-bool and positive"
    if not finite_nonnegative(parsed.get("clock_resolution_seconds")):
        return "clock_resolution_seconds must be finite and non-negative"
    if case["kind"] == "io":
        for key in ("setup_seconds", "read_seconds", "assemble_seconds", "write_seconds"):
            if not finite_nonnegative(parsed.get(key)):
                return f"{key} must be finite and non-negative"
        for key in ("bytes", "read_calls", "completions", "peak_in_flight", "application_copy_bytes"):
            if not integer_value(parsed.get(key)):
                return f"{key} must be a non-negative integer"
        if parsed.get("size") != case["bytes"] or parsed.get("rounds") != case["rounds"] or parsed.get("depth") != case["depth"]:
            return "JSON IO inputs do not match case"
        if parsed.get("bytes") != case["bytes"] * case["rounds"]:
            return "JSON IO byte count does not match size * rounds"
    elif case["kind"] == "alloc":
        for key in ("setup_seconds", "loop_seconds", "cleanup_seconds"):
            if not finite_nonnegative(parsed.get(key)):
                return f"{key} must be finite and non-negative"
        for key in ("items", "upstream_allocations", "upstream_deallocations", "upstream_bytes",
                    "peak_upstream_bytes", "observed_address_reuse", "checksum"):
            if not integer_value(parsed.get(key)):
                return f"{key} must be a non-negative integer"
        if parsed.get("items") != case["items"]:
            return "JSON alloc inputs do not match case"
        if parsed.get("checksum") != case["items"] * (case["items"] - 1) // 2:
            return "JSON alloc checksum does not match protocol"
        if parsed.get("upstream_allocations") != parsed.get("upstream_deallocations"):
            return "JSON alloc upstream balance failed"
    else:
        return "unknown case kind"
    return ""


def classify(raw: dict[str, Any], case: dict[str, Any]) -> tuple[str, dict[str, Any] | None, str]:
    clean = not (raw.get("timeout") or raw.get("error") or raw.get("cleanup_error"))
    combined = raw.get("stdout", "") + "\n" + raw.get("stderr", "")
    if clean and raw.get("exit_code") == 77 and "SKIP:" in combined:
        return "SKIP", None, ""
    if clean and raw.get("exit_code") == 0:
        if raw.get("stderr", "").strip():
            return "UNKNOWN", None, "stderr must be empty for a valid sample"
        parsed, error = parse_json_stdout(raw.get("stdout", ""))
        if error:
            return "UNKNOWN", None, error
        error = validate_parsed(parsed, case)
        if not error:
            return "VALID", parsed, ""
        return "UNKNOWN", parsed, error
    return "FAIL", None, ""


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def summarize(samples: list[dict[str, Any]]) -> dict[str, Any]:
    valid = [sample["parsed"] for sample in samples if sample["verdict"] == "VALID" and not sample["warmup"]]
    if len(valid) != 5:
        return {"valid_count": len(valid), "status": "NO_STATISTICS"}
    seconds = [float(sample["seconds"]) for sample in valid]
    summary: dict[str, Any] = {
        "valid_count": 5,
        "median_seconds": statistics.median(seconds),
        "min_seconds": min(seconds),
        "max_seconds": max(seconds),
        "stdev_seconds": statistics.stdev(seconds) if len(seconds) > 1 else 0.0,
        "status": "OK",
    }
    for key in ("kind", "variant", "items", "size", "rounds", "depth"):
        if key in valid[0]:
            summary[key] = valid[0][key]
    return summary


def run_phase(args: argparse.Namespace) -> int:
    repo_root = args.repo_root.resolve()
    exe = args.exe.resolve()
    if not exe.exists():
        raise SystemExit(f"missing executable: {exe}")
    if args.output.exists():
        raise SystemExit("--output must name a new directory")
    args.output.mkdir(parents=True)

    start_fingerprint = fingerprint(repo_root, exe)
    if start_fingerprint["missing_sources"]:
        raise SystemExit("missing fingerprint inputs: " + ", ".join(start_fingerprint["missing_sources"]))

    baseline: dict[str, Any] | None = None
    if args.phase == "compare":
        if args.baseline_result is None:
            raise SystemExit("--baseline-result is required for compare")
        baseline = json.loads(args.baseline_result.read_text(encoding="utf-8"))
        if baseline.get("phase") != "baseline" or not baseline.get("phase_valid"):
            raise SystemExit("baseline result is not valid")
        if baseline.get("fingerprint_start") != baseline.get("fingerprint_end"):
            raise SystemExit("baseline source changed during collection")
        if baseline.get("fingerprint_start") != start_fingerprint:
            raise SystemExit("compare executable/source fingerprint differs from baseline")

    planned = cases(args.phase)

    samples_dir = args.output / "samples"
    samples_dir.mkdir()
    case_summaries: list[dict[str, Any]] = []
    raw_index: list[dict[str, Any]] = []
    phase_valid = True
    blocked = False

    samples_by_case: dict[str, list[dict[str, Any]]] = {case_id(case): [] for case in planned}

    def run_one(case: dict[str, Any], sample_id: str, warmup: bool) -> dict[str, Any]:
        cid = case_id(case)
        raw = supervise(command(exe, case), args.timeout)
        verdict, parsed, parse_error = classify(raw, case)
        sample_record = {
            "sample_id": sample_id,
            "case_id": cid,
            "case": case,
            "warmup": warmup,
            "verdict": verdict,
            "parse_error": parse_error,
            "seconds": parsed.get("seconds") if parsed else None,
            "parsed": parsed,
            "raw": raw,
        }
        write_json(samples_dir / f"{sample_id}.json", sample_record)
        raw_index.append({k: sample_record[k] for k in ("sample_id", "case_id", "warmup", "verdict", "seconds")})
        samples_by_case[cid].append(sample_record)
        return sample_record

    for case in random.Random(args.seed).sample(planned, len(planned)):
        record = run_one(case, f"{case_id(case)}-warmup", True)
        if record["verdict"] != "VALID":
            phase_valid = False
            blocked = True
            break
    if not blocked:
        for round_index in range(1, 6):
            order = list(planned)
            random.Random(args.seed + round_index).shuffle(order)
            for case in order:
                run_one(case, f"{case_id(case)}-sample-{round_index}", False)

    for case in planned:
        cid = case_id(case)
        if samples_by_case[cid]:
            case_summaries.append({"case_id": cid, "case": case, "summary": summarize(samples_by_case[cid])})

    end_fingerprint = fingerprint(repo_root, exe)
    if end_fingerprint != start_fingerprint:
        phase_valid = False

    for entry in case_summaries:
        if entry["summary"]["status"] != "OK":
            phase_valid = False

    result = {
        "phase": args.phase,
        "seed": args.seed,
        "phase_valid": phase_valid,
        "blocked": blocked,
        "fingerprint_start": start_fingerprint,
        "fingerprint_end": end_fingerprint,
        "baseline_result": str(args.baseline_result) if args.baseline_result else None,
        "cases": case_summaries,
        "raw_index": raw_index,
        "notes": [
            "One warmup process is recorded per case and excluded from the five formal samples.",
            "ROUNDS inside B01_costs_benchmark is one process-local timed workload, not multiple independent samples.",
            "SKIP, FAIL and UNKNOWN samples are evidence only and never enter performance statistics.",
        ],
    }
    write_json(args.output / "result.json", result)
    print(json.dumps({
        "phase": args.phase,
        "phase_valid": phase_valid,
        "cases": len(case_summaries),
        "samples": len(raw_index),
        "output": str(args.output / "result.json"),
    }, ensure_ascii=False, sort_keys=True))
    return 0 if phase_valid else 1


def self_check() -> int:
    fake = """#!/usr/bin/env python3
import json
from pathlib import Path
import sys
mode = sys.argv[1]
state_path = Path(__file__).with_suffix(".state.json")
state = json.loads(state_path.read_text()) if state_path.exists() else {}
key = "-".join(sys.argv[1:])
count = state.get(key, 0)
state[key] = count + 1
state_path.write_text(json.dumps(state))
if mode == "alloc" and sys.argv[2] == "heap" and sys.argv[3] == "4096":
    print(json.dumps({"kind":"alloc","variant":"heap","items":4096,"seconds":0.004,
        "setup_seconds":0.0,"loop_seconds":0.003,"cleanup_seconds":0.001,
        "upstream_allocations":1,"upstream_deallocations":1,"upstream_bytes":4096,
        "peak_upstream_bytes":4096,"observed_address_reuse":0,"checksum":8386560,
        "clock_resolution_seconds":1e-9,"valid":True}))
    raise SystemExit(0)
if mode == "alloc" and sys.argv[2] == "arena":
    if count == 0:
        print(json.dumps({"kind":"alloc","variant":"arena","items":4096,"seconds":0.004,
            "setup_seconds":0.0,"loop_seconds":0.003,"cleanup_seconds":0.001,
            "upstream_allocations":1,"upstream_deallocations":1,"upstream_bytes":4096,
            "peak_upstream_bytes":4096,"observed_address_reuse":0,"checksum":8386560,
            "clock_resolution_seconds":1e-9,"valid":True}))
        raise SystemExit(0)
    print(json.dumps({"kind":"alloc","variant":"arena","items":4096,"seconds":True,
        "setup_seconds":0.0,"loop_seconds":0.003,"cleanup_seconds":0.001,
        "upstream_allocations":1,"upstream_deallocations":0,"upstream_bytes":4096,
        "peak_upstream_bytes":4096,"observed_address_reuse":0,"checksum":1,
        "clock_resolution_seconds":1e-9,"valid":True}))
    raise SystemExit(0)
if mode == "alloc" and sys.argv[2] == "pool":
    print(json.dumps({"kind":"alloc","variant":"pool","items":4096,"seconds":0.004,
        "setup_seconds":0.0,"loop_seconds":0.003,"cleanup_seconds":0.001,
        "upstream_allocations":1,"upstream_deallocations":1,"upstream_bytes":4096,
        "peak_upstream_bytes":4096,"observed_address_reuse":0,"checksum":8386560,
        "clock_resolution_seconds":1e-9,"valid":True}))
    if count > 0:
        print("hidden failure", file=sys.stderr)
    raise SystemExit(0)
if mode == "io" and sys.argv[2] == "buffered" and sys.argv[3] == "4096":
    print(json.dumps({"kind":"io","variant":"buffered","size":4096,"rounds":2,"depth":4,
        "seconds":0.002,"setup_seconds":0.0,"read_seconds":0.001,"assemble_seconds":0.0,
        "write_seconds":0.001,"bytes":8192,"read_calls":2,"completions":0,"peak_in_flight":0,
        "application_copy_bytes":8192,"clock_resolution_seconds":1e-9,"valid":True}))
    raise SystemExit(0)
if mode == "io" and sys.argv[2] == "mapped":
    if count == 0:
        print(json.dumps({"kind":"io","variant":"mapped","size":4096,"rounds":2,"depth":4,
            "seconds":0.002,"setup_seconds":0.0,"read_seconds":0.001,"assemble_seconds":0.0,
            "write_seconds":0.001,"bytes":8192,"read_calls":2,"completions":0,"peak_in_flight":0,
            "application_copy_bytes":8192,"clock_resolution_seconds":1e-9,"valid":True}))
        raise SystemExit(0)
    print('{"kind":"io","variant":"mapped","size":4096,"rounds":2,"depth":4,"seconds":NaN,"valid":true}')
    raise SystemExit(0)
if mode == "io" and sys.argv[2] == "completion":
    if count == 0:
        print(json.dumps({"kind":"io","variant":"completion","size":4096,"rounds":2,"depth":4,
            "seconds":0.002,"setup_seconds":0.0,"read_seconds":0.001,"assemble_seconds":0.0,
            "write_seconds":0.001,"bytes":8192,"read_calls":2,"completions":0,"peak_in_flight":0,
            "application_copy_bytes":8192,"clock_resolution_seconds":1e-9,"valid":True}))
        raise SystemExit(0)
    print(json.dumps({"kind":"io","variant":"completion","size":999,"rounds":2,"depth":4,
        "seconds":0.002,"setup_seconds":-1.0,"read_seconds":0.001,"assemble_seconds":0.0,
        "write_seconds":0.001,"bytes":8192,"read_calls":2,"completions":0,"peak_in_flight":0,
        "application_copy_bytes":8192,"clock_resolution_seconds":False,"valid":True}))
    raise SystemExit(0)
if mode == "io":
    print(json.dumps({"kind":"io","variant":sys.argv[2],"size":int(sys.argv[3]),"rounds":int(sys.argv[4]),
        "depth":int(sys.argv[5]),"seconds":0.001,"setup_seconds":0.0,"read_seconds":0.001,
        "assemble_seconds":0.0,"write_seconds":0.0,"bytes":int(sys.argv[3])*int(sys.argv[4]),
        "read_calls":1,"completions":0,"peak_in_flight":0,"application_copy_bytes":0,
        "clock_resolution_seconds":1e-9,"valid":True}))
    raise SystemExit(0)
print("broken sample", file=sys.stderr)
raise SystemExit(1)
"""
    with tempfile.TemporaryDirectory(prefix="c07-b01-sampler-check-") as tmp:
        root = Path(tmp) / "repo"
        out = Path(tmp) / "out"
        exe = Path(tmp) / ("fake_b01.py")
        root.mkdir()
        for name in SOURCE_FILES:
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(name + "\n", encoding="utf-8")
        exe.write_text(fake, encoding="utf-8")
        if os.name != "nt":
            exe.chmod(0o755)
        ns = argparse.Namespace(exe=Path(sys.executable), output=out, phase="baseline", seed=20260910,
                                baseline_result=None, repo_root=root, timeout=5.0)
        old_command = command
        old_cases = cases
        old_random = random.Random
        class NoShuffle:
            def __init__(self, _seed: int) -> None:
                pass
            def shuffle(self, _values: list[dict[str, Any]]) -> None:
                pass
            def sample(self, values: list[dict[str, Any]], _count: int) -> list[dict[str, Any]]:
                return list(values)
        try:
            globals()["command"] = lambda _exe, case: [sys.executable, str(exe), *old_command(_exe, case)[1:]]
            globals()["cases"] = lambda _phase: [
                {"kind": "alloc", "variant": "heap", "items": 4096},
                {"kind": "io", "variant": "buffered", "bytes": 4096, "rounds": 2, "depth": 4},
                {"kind": "io", "variant": "mapped", "bytes": 4096, "rounds": 2, "depth": 4},
                {"kind": "io", "variant": "completion", "bytes": 4096, "rounds": 2, "depth": 4},
                {"kind": "alloc", "variant": "arena", "items": 4096},
                {"kind": "alloc", "variant": "pool", "items": 4096},
            ]
            random.Random = NoShuffle
            rc = run_phase(ns)
            blocked_out = Path(tmp) / "blocked"
            globals()["cases"] = lambda _phase: [{"kind": "alloc", "variant": "heap", "items": 32768}]
            ns.output = blocked_out
            blocked_rc = run_phase(ns)
        finally:
            globals()["command"] = old_command
            globals()["cases"] = old_cases
            random.Random = old_random
        result = json.loads((out / "result.json").read_text(encoding="utf-8"))
        heap = next(case for case in result["cases"] if case["case_id"] == "alloc-heap-4096")
        buffered = next(case for case in result["cases"] if case["case_id"] == "io-buffered-4096-r2-d4")
        mapped = next(case for case in result["cases"] if case["case_id"] == "io-mapped-4096-r2-d4")
        bad_alloc = next(case for case in result["cases"] if case["case_id"] == "alloc-arena-4096")
        hidden = next(case for case in result["cases"] if case["case_id"] == "alloc-pool-4096")
        assert rc == 1
        assert heap["summary"]["valid_count"] == 5
        assert buffered["summary"]["valid_count"] == 5
        assert mapped["summary"]["valid_count"] == 0
        assert bad_alloc["summary"]["valid_count"] == 0
        assert hidden["summary"]["valid_count"] == 0
        assert result["blocked"] is False
        assert len([entry for entry in result["raw_index"] if entry["case_id"] == "alloc-heap-4096"]) == 6
        assert len([entry for entry in result["raw_index"] if entry["case_id"] == "alloc-heap-4096"
                    and entry["warmup"]]) == 1
        assert any(entry["case_id"] == "io-mapped-4096-r2-d4" and entry["verdict"] == "UNKNOWN"
                   for entry in result["raw_index"])
        assert any(entry["case_id"] == "io-completion-4096-r2-d4" and entry["verdict"] == "UNKNOWN"
                   for entry in result["raw_index"])
        assert any(entry["case_id"] == "alloc-arena-4096" and entry["verdict"] == "UNKNOWN"
                   for entry in result["raw_index"])
        assert any(entry["case_id"] == "alloc-pool-4096" and entry["verdict"] == "UNKNOWN"
                   for entry in result["raw_index"])
        blocked_result = json.loads((blocked_out / "result.json").read_text(encoding="utf-8"))
        assert blocked_rc == 1
        assert blocked_result["blocked"] is True
        assert len(blocked_result["raw_index"]) == 1
        assert blocked_result["raw_index"][0]["warmup"] is True
        assert blocked_result["raw_index"][0]["verdict"] == "FAIL"
    print("sample_benchmarks.py self-check passed")
    return 0


def default_repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--phase", choices=("baseline", "compare"), default="baseline")
    parser.add_argument("--seed", type=int, default=20260910)
    parser.add_argument("--baseline-result", type=Path)
    parser.add_argument("--repo-root", type=Path, default=default_repo_root())
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    if args.exe is None or args.output is None:
        parser.error("--exe and --output are required unless --self-check is used")
    return run_phase(args)


if __name__ == "__main__":
    raise SystemExit(main())
