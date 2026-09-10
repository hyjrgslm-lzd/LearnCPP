from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import statistics
import sys
from typing import Any

sys.dont_write_bytecode = True
REPO = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(REPO / "C01_Build_Compile_Link" / "exercises" / "tools"))
from process_runner import run_process  # noqa: E402

PIPELINE_SCENARIOS = ("small_low_valid", "small_high_valid", "large_low_valid", "large_high_valid")
PIPELINE_VARIANTS = ("loop", "materialized", "reparse")
PIPELINE_INSTRUMENTS = ("timed", "counted")
INDEX_SCENARIOS = ("small_low_hit", "small_high_hit", "large_low_hit", "large_high_hit")
INDEX_VARIANTS = ("linear_vector", "sorted_vector", "map", "unordered_map")
INDEX_INSTRUMENTS = ("timed", "counted")


def digest_file(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def digest_tree(root: Path) -> str:
    result = hashlib.sha256()
    for directory, directories, files in os.walk(root):
        directories[:] = sorted(d for d in directories if d not in {"__pycache__", "notes"} and not d.startswith("build"))
        for name in sorted(files):
            path = Path(directory) / name
            if path.suffix.lower() not in {".cpp", ".hpp", ".txt", ".md", ".cmake", ".py"}:
                continue
            result.update(path.relative_to(root).as_posix().encode("utf-8"))
            result.update(bytes.fromhex(digest_file(path)))
    return result.hexdigest()


def ensure_new_dir(path: Path) -> None:
    if path.exists():
        raise FileExistsError(f"output already exists: {path}")
    path.mkdir(parents=True)


def write_process(raw: Path, name: str, result: dict[str, Any]) -> None:
    (raw / f"{name}.stdout.txt").write_text(result["stdout"], encoding="utf-8", errors="replace")
    (raw / f"{name}.stderr.txt").write_text(result["stderr"], encoding="utf-8", errors="replace")
    meta = {k: v for k, v in result.items() if k not in {"stdout", "stderr"}}
    (raw / f"{name}.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def scenario_size(name: str) -> int:
    return 256 if name.startswith("small_") else 8192


def build_schedule(samples: int, warmups: int, seed: int, smoke: bool) -> list[dict[str, Any]]:
    if smoke:
        return [
            {"phase": "warmup", "rep": 0, "case": "pipeline", "scenario": "small_low_valid", "variant": "loop", "instrument": "timed"},
            {"phase": "sample", "rep": 0, "case": "pipeline", "scenario": "small_low_valid", "variant": "loop", "instrument": "timed"},
            {"phase": "sample", "rep": 0, "case": "pipeline", "scenario": "small_low_valid", "variant": "loop", "instrument": "counted"},
            {"phase": "sample", "rep": 0, "case": "pipeline", "scenario": "small_low_valid", "variant": "materialized", "instrument": "timed"},
            {"phase": "sample", "rep": 0, "case": "pipeline", "scenario": "small_low_valid", "variant": "reparse", "instrument": "timed"},
            {"phase": "sample", "rep": 0, "case": "index", "scenario": "small_low_hit", "variant": "linear_vector", "instrument": "counted"},
            {"phase": "sample", "rep": 0, "case": "index", "scenario": "small_low_hit", "variant": "unordered_map", "instrument": "timed"},
        ]

    schedule: list[dict[str, Any]] = []
    rng = random.Random(seed)
    for phase, repetitions in (("warmup", warmups), ("sample", samples)):
        for rep in range(repetitions):
            items: list[dict[str, Any]] = []
            for scenario in PIPELINE_SCENARIOS:
                for variant in PIPELINE_VARIANTS:
                    for instrument in PIPELINE_INSTRUMENTS:
                        items.append({"phase": phase, "rep": rep, "case": "pipeline", "scenario": scenario,
                                      "variant": variant, "instrument": instrument})
            for scenario in INDEX_SCENARIOS:
                for variant in INDEX_VARIANTS:
                    for instrument in INDEX_INSTRUMENTS:
                        items.append({"phase": phase, "rep": rep, "case": "index", "scenario": scenario,
                                      "variant": variant, "instrument": instrument})
            rng.shuffle(items)
            schedule.extend(items)
    return schedule


def run_one(exe: Path, raw: Path, timeout: float, seed: int, item: dict[str, Any]) -> dict[str, Any]:
    sample_seed = seed + item["rep"] * 1009 + scenario_size(item["scenario"])
    command = [str(exe), "--bench-one", "--case", item["case"], "--scenario", item["scenario"],
               "--variant", item["variant"], "--seed", str(sample_seed)]
    command += ["--instrument", item["instrument"]]
    name = f"{item['phase']}-{item['rep']:02d}-{item['case']}-{item['scenario']}-{item['variant']}-{item['instrument']}"
    result = run_process(command, timeout)
    write_process(raw, name, result)
    payload: dict[str, Any] | None = None
    verdict = "FAIL"
    failure = ""
    if result["status"] == "PASS":
        try:
            payload = json.loads(result["stdout"])
            verdict = "PASS"
            if payload.get("checksum") != payload.get("oracle_checksum"):
                verdict = "FAIL"
                failure = "checksum differs from independent oracle"
            elif item["case"] == "pipeline" and (
                payload.get("valid_records") != payload.get("oracle_valid_records")
                or payload.get("error_records") != payload.get("oracle_error_records")
            ):
                verdict = "FAIL"
                failure = "pipeline counts differ from independent oracle"
            elif item["case"] == "index" and payload.get("matches") != payload.get("oracle_matches"):
                verdict = "FAIL"
                failure = "index matches differ from independent oracle"
        except json.JSONDecodeError as error:
            failure = f"invalid json: {error}"
    else:
        failure = result["error"] or result["cleanup_error"] or f"exit {result['exit_code']}"
    return {**item, "seed": sample_seed, "command": command, "process": {k: result[k] for k in
            ("exit_code", "timeout", "process_seconds", "status", "error", "cleanup_error")},
            "payload": payload, "verdict": verdict, "failure": failure}


def current_hashes(exe: Path) -> dict[str, str]:
    return {
        "source_sha256": digest_tree(REPO / "C06_Ranges" / "exercises" / "B01_cost"),
        "capstone1_source_sha256": digest_tree(REPO / "C06_Ranges" / "exercises" / "CAPSTONE1_log_pipeline"),
        "exe_sha256": digest_file(exe),
        "driver_sha256": digest_file(Path(__file__).resolve()),
        "process_runner_sha256": digest_file(REPO / "C01_Build_Compile_Link" / "exercises" / "tools" / "process_runner.py"),
    }


def hash_drift(before: dict[str, str], after: dict[str, str]) -> dict[str, dict[str, str]]:
    return {key: {"before": before[key], "after": after.get(key, "")}
            for key in before if before[key] != after.get(key)}


def drift_self_check() -> None:
    changed = hash_drift({"exe_sha256": "a", "driver_sha256": "b"}, {"exe_sha256": "a", "driver_sha256": "c"})
    if set(changed) != {"driver_sha256"}:
        raise AssertionError("drift self-check failed")


def summarize(rounds: list[dict[str, Any]], samples: int) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    keys = sorted({(r["case"], r["scenario"], r["variant"], r["instrument"]) for r in rounds})
    for case, scenario, variant, instrument in keys:
        formal = [r for r in rounds if r["phase"] == "sample" and (r["case"], r["scenario"], r["variant"], r["instrument"]) == (case, scenario, variant, instrument)]
        valid = [r for r in formal if r["verdict"] == "PASS" and r.get("payload")]
        entry: dict[str, Any] = {"case": case, "scenario": scenario, "variant": variant,
                                 "instrument": instrument, "valid_samples": len(valid),
                                 "requested_samples": samples}
        if len(valid) == samples:
            if case == "pipeline":
                values = [r["payload"]["elapsed_ns"] / 1_000_000 for r in valid]
                entry.update({"elapsed_ms": values, "median_ms": statistics.median(values),
                              "min_ms": min(values), "max_ms": max(values)})
            else:
                build = [r["payload"]["build_ns"] / 1_000_000 for r in valid]
                lookup = [r["payload"]["lookup_ns"] / 1_000_000 for r in valid]
                entry.update({"build_ms": build, "lookup_ms": lookup,
                              "build_median_ms": statistics.median(build),
                              "lookup_median_ms": statistics.median(lookup)})
        else:
            entry["status"] = "INVALID"
        result.append(entry)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Run B01_cost benchmark processes.")
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--samples", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--drift-self-check", action="store_true")
    args = parser.parse_args()

    if args.drift_self_check:
        drift_self_check()
        print("B01 drift self-check OK")
        return 0

    if args.samples < 1 or args.warmups < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("samples, warmups and timeout must be positive")
    if args.exe is None or args.output is None:
        parser.error("--exe and --output are required unless --drift-self-check is used")
    exe = args.exe.resolve()
    if not exe.exists():
        parser.error(f"exe not found: {exe}")
    output = args.output.resolve()
    ensure_new_dir(output)
    raw = output / "raw"
    raw.mkdir()

    schedule = build_schedule(args.samples, args.warmups, args.seed, args.smoke)
    before_hashes = current_hashes(exe)
    report: dict[str, Any] = {
        "schema": 1,
        "status": "RUNNING",
        "formal": not args.smoke,
        "repo": str(REPO),
        "environment": {"python": sys.version, "platform": platform.platform(),
                         "machine": platform.machine(), "logical_cpus": os.cpu_count()},
        "arguments": {"samples": args.samples, "warmups": args.warmups, "seed": args.seed,
                       "timeout": args.timeout, "exe": str(exe), "smoke": args.smoke},
        **before_hashes,
        "schedule": schedule,
        "rounds": [],
        "summary": [],
    }

    for item in schedule:
        report["rounds"].append(run_one(exe, raw, args.timeout, args.seed, item))
        report["summary"] = summarize(report["rounds"], 1 if args.smoke else args.samples)
        (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    after_hashes = current_hashes(exe)
    for key, value in after_hashes.items():
        report[f"{key}_after"] = value
    report["hash_drift"] = hash_drift(before_hashes, after_hashes)
    failures = [item for item in report["rounds"] if item["verdict"] != "PASS"]
    invalid = [item for item in report["summary"] if item.get("valid_samples") != item.get("requested_samples")]
    report["status"] = "FAIL" if failures or invalid or report["hash_drift"] else "PASS"
    (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "output": str(output), "formal": report["formal"]}, ensure_ascii=False, indent=2))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
