"""Run bounded course checks; crashes and timeouts are never expected rejections."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process


def run(command: list[str], timeout: float, expected: str = "", allow_skip: bool = False) -> int:
    result = run_process(command, timeout)
    sys.stdout.write(result["stdout"])
    sys.stderr.write(result["stderr"])
    for field in ("timeout", "error", "cleanup_error"):
        if result[field]:
            print(f"run_check: FAIL {field}={result[field]}", file=sys.stderr)
            return 1
    if allow_skip and result["exit_code"] == 77 and not expected:
        return 77
    if expected:
        ok = result["exit_code"] == 1 and expected in result["stdout"] + result["stderr"]
    else:
        ok = result["exit_code"] == 0
    if not ok:
        print(f"run_check: FAIL exit={result['exit_code']}", file=sys.stderr)
    return 0 if ok else 1


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--expect-failure", default="")
    parser.add_argument("--allow-skip", action="store_true")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or args.timeout <= 0:
        parser.error("a command and positive timeout are required")
    return run(command, args.timeout, args.expect_failure, args.allow_skip)


if __name__ == "__main__":
    raise SystemExit(main())
