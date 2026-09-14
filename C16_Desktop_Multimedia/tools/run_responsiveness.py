"""Collect one warm-up and five independent Qt processes per version/workload."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import platform
import random
import statistics
import sys
from datetime import datetime, timezone

from run_check import run_process

COURSE = Path(__file__).resolve().parents[1]


def fingerprint(paths: list[Path]) -> dict[str, str]:
    return {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--interference-note", default="Other machine activity was not measured.")
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((COURSE / "build").resolve()) or output.exists():
        parser.error("choose a new output directory beneath the course build directory")
    suffix = ".exe" if sys.platform == "win32" else ""
    executables = {name: args.bin_dir.resolve() / f"c16_l15_{name}{suffix}"
                   for name in ("per_event", "coalesced")}
    if not all(path.is_file() for path in executables.values()):
        parser.error("build both L15 observation executables first")
    sources = sorted((COURSE / "exercises/L15_responsiveness").rglob("*.cpp"))
    sources += sorted((COURSE / "exercises/L15_responsiveness").rglob("*.hpp"))
    sources += [Path(__file__).resolve(), COURSE / "tools/run_check.py",
                COURSE.parent / "C01_Build_Compile_Link/exercises/tools/process_runner.py"]
    files = sources + list(executables.values())
    before = fingerprint(files)
    output.mkdir(parents=True)
    rng = random.Random(16015)
    rows: list[dict] = []
    for count in (1000, 30000):
        for iteration in range(6):
            order = list(executables)
            rng.shuffle(order)
            for version in order:
                command = [str(executables[version]), str(count)]
                process = run_process(command, 30)
                row = {"version": version, "count": count, "iteration": iteration,
                       "warmup": iteration == 0, "process": process}
                valid = process["status"] == "PASS"
                try:
                    observation = json.loads(process["stdout"])
                    expected = ((count - 1) * 2654435761) ^ 17
                    expected_callbacks = count if version == "per_event" else 1
                    valid = valid and observation["final"] == expected
                    valid = valid and observation["submitted"] == count
                    valid = valid and observation["callbacks"] == expected_callbacks
                    valid = valid and observation["peak_pending"] == expected_callbacks
                    valid = valid and observation["post_ns"] > 0 and observation["drain_ns"] > 0
                    row["observation"] = observation
                except (ValueError, KeyError, TypeError):
                    valid = False
                row["valid"] = bool(valid)
                rows.append(row)
                (output / f"run-{len(rows):02}.json").write_text(
                    json.dumps(row, ensure_ascii=False, indent=2), encoding="utf-8")
    after = fingerprint(files)
    stable = before == after
    summary = {"captured_at": datetime.now(timezone.utc).isoformat(), "platform": platform.platform(),
               "python": sys.version, "seed": 16015, "interference": args.interference_note,
               "clock": "QElapsedTimer monotonic nanoseconds; nanoseconds are units, not guaranteed resolution",
               "timing": "post includes producer start, every preview calculation/post and join; drain includes all owned MetaCall callbacks; Qt startup and validation excluded",
               "before": before, "after": after, "sources_and_binaries_stable": stable,
               "valid_runs": sum(row["valid"] for row in rows), "total_runs": len(rows), "groups": []}
    for count in (1000, 30000):
        for version in executables:
            selected = [row for row in rows if row["count"] == count and row["version"] == version
                        and not row["warmup"]]
            group = {"version": version, "count": count, "valid": stable and all(row["valid"] for row in selected)}
            if group["valid"]:
                for stage in ("post_ns", "drain_ns"):
                    values = [row["observation"][stage] for row in selected]
                    group[stage] = {"samples": values, "median": statistics.median(values),
                                    "min": min(values), "max": max(values), "pstdev": statistics.pstdev(values)}
            summary["groups"].append(group)
    summary["status"] = "PASS" if stable and all(row["valid"] for row in rows) else "FAIL"
    (output / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"status": summary["status"], "valid_runs": summary["valid_runs"],
                      "total_runs": len(rows), "output": str(output)}, ensure_ascii=False))
    return 0 if summary["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
