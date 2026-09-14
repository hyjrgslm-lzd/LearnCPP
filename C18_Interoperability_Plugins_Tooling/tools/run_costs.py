"""Warm up and retain five independent process samples per scenario and variant."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import platform
import random
import statistics
import sys

sys.dont_write_bytecode = True
from run_check import run_process


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--variants", nargs="+", default=["per_record"])
    args = parser.parse_args()
    if any(v not in {"per_record", "batch", "copy_batch"} for v in args.variants):
        parser.error("unknown variant")
    executable = args.exe.resolve()
    course = Path(__file__).resolve().parents[1]
    destination = args.output.resolve()
    try:
        destination.relative_to(course / "build")
    except ValueError:
        parser.error("output must stay in this course's ignored build directory")
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        parser.error("retain old evidence: choose a new output filename")
    bound = [executable, course / "exercises/B01_boundary_cost/baseline.cpp", course / "exercises/core/c18/bytes.hpp", course / "exercises/include/c18/abi.h"]
    fingerprints = {str(path): digest(path) for path in bound}
    samples = []
    jobs = [(size, 65536 // size, variant, sample) for size in (16, 1024, 65536)
            for variant in args.variants for sample in range(5)]
    random.Random(1801).shuffle(jobs)
    warmups = [(size, 65536 // size, variant, -1) for size in (16, 1024, 65536) for variant in args.variants]
    for size, records, variant, sample in warmups + jobs:
        command = [str(executable), str(size), str(records), "1024"]
        if variant != "per_record":
            command.append(variant)
        result = run_process(command, 45)
        observation = {"warmup": sample == -1, "sample": sample, "requested_variant": variant, "process": result}
        if result["status"] == "PASS":
            try:
                measurement = json.loads(result["stdout"])
                expected_calls = records * 1024 if variant == "per_record" else 1024
                if not isinstance(measurement, dict) or any(measurement.get(key) != value for key, value in {
                    "variant": variant, "record_size": size, "records": records, "iterations": 1024,
                    "calls": expected_calls, "completed_bytes": 65536 * 1024,
                }.items()):
                    raise ValueError("driver returned a different workload, variant, or call count")
                seconds = measurement.get("seconds")
                if isinstance(seconds, bool) or not isinstance(seconds, (int, float)) or not math.isfinite(seconds) or seconds <= 0:
                    raise ValueError("invalid elapsed time")
                observation["measurement"] = measurement
            except (ValueError, KeyError, TypeError) as error:
                observation["parse_error"] = str(error)
        samples.append(observation)
    stable = all(digest(path) == fingerprints[str(path)] for path in bound)
    ok = stable and all(s["process"]["status"] == "PASS" and "parse_error" not in s for s in samples)
    summary = []
    for size in (16, 1024, 65536):
        for variant in args.variants:
            values = [s["measurement"]["seconds"] for s in samples if not s["warmup"] and "parse_error" not in s and
                      s.get("measurement", {}).get("record_size") == size and s["requested_variant"] == variant]
            if len(values) == 5:
                summary.append({"record_size": size, "variant": variant, "median": statistics.median(values),
                                "min": min(values), "max": max(values), "samples": values})
    report = {"status": "PASS" if ok else "FAIL", "platform": platform.platform(), "python": sys.version,
              "source_and_binary": fingerprints, "unchanged_during_measurement": stable,
              "order_seed": 1801, "interference": "Desktop/background workload is not isolated; retain dispersion and do not generalize timings.",
              "samples": samples, "summary": summary}
    with destination.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
    print(json.dumps({"status": report["status"], "summary": summary}, ensure_ascii=False, indent=2))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
