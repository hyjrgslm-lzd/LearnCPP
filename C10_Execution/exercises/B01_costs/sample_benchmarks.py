"""Independent process samples; no filtering of inconvenient measurements."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import random
import statistics
import sys

sys.dont_write_bytecode = True
REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--note", required=True, help="Record observed concurrent work and platform limits")
    args = parser.parse_args()
    if args.output.exists():
        parser.error("use a fresh output directory")
    binary = args.binary.resolve()
    source = Path(__file__).with_name("main.cpp")
    fingerprints = {"binary": digest(binary), "source": digest(source)}
    args.output.mkdir(parents=True)
    rng = random.Random(20260911)
    samples = []
    failed = False
    for size in (32768, 65536):
        for work in (512, 2048):
            for repetition in range(6):
                order = ["serial", "per_item", "bulk"]
                rng.shuffle(order)
                checksums = set()
                for variant in order:
                    if digest(binary) != fingerprints["binary"] or digest(source) != fingerprints["source"]:
                        raise RuntimeError("measured source/binary changed; keep failed run and start a new one")
                    command = [str(binary), "--bench", variant, str(size), str(work), "4242"]
                    raw = run_process(command, 30)
                    parsed = []
                    try:
                        parsed = [json.loads(line) for line in raw.get("stdout", "").splitlines() if line.startswith("{")]
                    except json.JSONDecodeError:
                        pass
                    valid = raw["status"] == "PASS" and len(parsed) == 1
                    if valid:
                        value = parsed[0]
                        valid = (value.get("variant") == variant and value.get("size") == size
                                 and value.get("work") == work and value.get("seconds", 0) > 0)
                        checksums.add(value.get("checksum"))
                    failed |= not valid
                    sample = {"size": size, "work": work, "variant": variant,
                              "phase": "warmup" if repetition == 0 else "sample",
                              "repetition": repetition, "order": order,
                              "valid": valid, "measurement": parsed[0] if len(parsed) == 1 else None,
                              "process": raw}
                    samples.append(sample)
                    (args.output / f"{size}-{work}-{repetition}-{variant}.json").write_text(
                        json.dumps(sample, indent=2) + "\n", encoding="utf-8")
                if len(checksums) != 1:
                    failed = True
    groups = []
    for size in (32768, 65536):
        for work in (512, 2048):
            for variant in ("serial", "per_item", "bulk"):
                selected = [s for s in samples if s["size"] == size and s["work"] == work
                            and s["variant"] == variant and s["phase"] == "sample"]
                complete = len(selected) == 5 and all(s["valid"] for s in selected)
                values = [s["measurement"]["seconds"] for s in selected] if complete else []
                groups.append({"size": size, "work": work, "variant": variant,
                               "count": len(values), "median": statistics.median(values) if values else None,
                               "min": min(values) if values else None, "max": max(values) if values else None,
                               "population_stddev": statistics.pstdev(values) if values else None})
    report = {"status": "FAIL" if failed else "PASS", "utc": datetime.now(timezone.utc).isoformat(),
              "platform": platform.platform(), "python": sys.version, "note": args.note,
              "fingerprints": fingerprints, "binary": str(binary), "order_seed": 20260911,
              "warmups": 12, "formal_samples": 60, "groups": groups,
              "interpretation": "Only total completion interval is comparable. pre_wait_seconds has variant-specific boundaries."
              }
    (args.output / "summary.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(report["status"], args.output)
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
