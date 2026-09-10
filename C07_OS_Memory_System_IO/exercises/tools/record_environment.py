"""Record explicit tool/hardware inputs, not the user's environment variables."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--build", type=Path)
    parser.add_argument("--uring-prefix", type=Path)
    parser.add_argument("--source-file", type=Path, action="append", default=[])
    args = parser.parse_args()
    if args.output.exists():
        parser.error("choose a new evidence file; existing records are immutable")
    record = {"time_utc": datetime.now(timezone.utc).isoformat(), "platform": platform.platform(),
              "machine": platform.machine(), "processor": platform.processor(), "cpu_count": os.cpu_count(),
              "python": sys.version, "cwd": str(Path.cwd()), "commands": [], "file_hashes": {},
              "clocks": {name: vars(time.get_clock_info(name)) for name in ("monotonic", "perf_counter")}}
    commands = [["cmake", "--version"], ["ctest", "--version"]]
    if sys.platform.startswith("linux"):
        commands += [["g++", "--version"], ["ldd", "--version"], ["uname", "-a"],
                     ["findmnt", "-T", str(Path.cwd())], ["lscpu", "-J"]]
    for command in commands:
        if not shutil.which(command[0]):
            record["commands"].append({"command": command, "available": False})
            continue
        result = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=15)
        record["commands"].append({"command": command, "exit_code": result.returncode,
                                   "stdout": result.stdout, "stderr": result.stderr})
    inputs = list(args.source_file)
    if args.build:
        cache = args.build / "CMakeCache.txt"
        if cache.exists():
            selected = [line for line in cache.read_text(encoding="utf-8", errors="replace").splitlines()
                        if line.startswith(("CMAKE_CXX_COMPILER:", "CMAKE_CXX_COMPILER_VERSION:",
                                            "CMAKE_CXX_FLAGS", "CMAKE_GENERATOR:", "CMAKE_GENERATOR_INSTANCE:",
                                            "CMAKE_BUILD_TYPE:", "C07_"))]
            record["cmake_inputs"] = selected
            inputs.append(cache)
    if args.uring_prefix:
        inputs += [args.uring_prefix / "c07-source-commit.txt",
                   args.uring_prefix / "include/liburing/io_uring_version.h",
                   args.uring_prefix / "include/liburing.h",
                   args.uring_prefix / "lib/liburing.so.2.15"]
    for path in inputs:
        if not path.is_file():
            raise FileNotFoundError(path)
        record["file_hashes"][str(path.resolve())] = hashlib.sha256(path.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"environment: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
