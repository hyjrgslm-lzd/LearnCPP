"""Record a bounded command using the existing supervisor; normalize Windows env."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if len(sys.argv) > 1 and sys.argv[1] == "--child":
    # Passing an explicit environment removes duplicate PATH/Path entries in a
    # native Windows host's inherited block without altering host configuration.
    raise SystemExit(subprocess.call(sys.argv[2:], env=dict(os.environ)))
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--timeout", type=float, default=120)
parser.add_argument("command", nargs=argparse.REMAINDER)
args = parser.parse_args()
command = args.command[1:] if args.command[:1] == ["--"] else args.command
if not command:
    parser.error("command required after --")
if args.output.exists():
    parser.error("output already exists; use a new record path")
result = run_process([sys.executable, str(Path(__file__).resolve()), "--child", *command], args.timeout)
result.update(requested_command=command, cwd=str(Path.cwd()))
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
for stream, output in ((sys.stdout, result["stdout"]), (sys.stderr, result["stderr"])):
    if len(output)>7000:
        print(f"Full output: {args.output}; showing final 7000 characters", file=stream)
    print(output[-7000:], end="", file=stream)
raise SystemExit(0 if result["status"] == "PASS" else 1)
