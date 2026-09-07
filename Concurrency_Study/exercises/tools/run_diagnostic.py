"""Bound an explicitly selected diagnostic process without treating UB as a test."""
import argparse
import math
import sys

from run_benchmarks import run_process


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=5)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("provide a finite positive timeout and a command after --")
    result = run_process(command, args.timeout)
    print(result["stdout"], end="")
    print(result["stderr"], end="", file=sys.stderr)
    if result["error"] or result["cleanup_error"]:
        print(result["error"], result["cleanup_error"], file=sys.stderr)
        return 1
    if result["timeout"]:
        print("TIMEOUT: diagnostic terminated; this is a failed run, not proof by timing.", file=sys.stderr)
        return 1
    print(f"Process exited {result['exit_code']}; an exit code alone does not establish correctness.", file=sys.stderr)
    return result["exit_code"] if result["exit_code"] in (0, 77) else 1


if __name__ == "__main__":
    raise SystemExit(main())
