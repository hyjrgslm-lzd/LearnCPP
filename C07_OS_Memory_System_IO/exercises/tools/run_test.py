"""Reuse the course process supervisor; expected rejection is not arbitrary failure."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import sys
import tempfile

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process


def supervise(command: list[str], timeout: float) -> dict:
    """Sequential course-tool entry: supervise both child processes and fixture files."""
    # The supervisor owns fixture files even when a student's check calls exit(1).
    # Only this process/its children see the overridden temporary-directory keys.
    old_temp = {key: os.environ.get(key) for key in ("TMP", "TEMP", "TMPDIR")}
    result = None
    try:
        with tempfile.TemporaryDirectory(prefix="c07-test-") as temporary:
            for key in old_temp:
                os.environ[key] = temporary
            result = run_process(command, timeout)
            result["temporary_directory"] = temporary
        result["temporary_directory_removed"] = True
    except OSError as error:
        if result is None:
            result = {"command": command, "exit_code": None, "timeout": False,
                      "stdout": "", "stderr": "", "cleanup_error": "",
                      "error": f"fixture setup failed: {error}", "status": "FAIL"}
        else:
            result["cleanup_error"] += f" fixture cleanup failed: {error}"
            result["temporary_directory_removed"] = False
    finally:
        for key, value in old_temp.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--records", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=25)
    parser.add_argument("--expect-exit", type=int, default=0)
    parser.add_argument("--contains", default="")
    parser.add_argument("--allow-skip", action="store_true")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("a command is required after --")
    result = supervise(command, args.timeout)
    clean = not (result["timeout"] or result["error"] or result["cleanup_error"])
    combined = result["stdout"] + "\n" + result["stderr"]
    skipped = clean and args.allow_skip and result["exit_code"] == 77 and "SKIP:" in combined
    passed = clean and result["exit_code"] == args.expect_exit and args.contains in combined
    verdict = "SKIP" if skipped else "PASS" if passed else "FAIL"
    result.update(name=args.name, cwd=str(Path.cwd()), expected_exit=args.expect_exit,
                  required_text=args.contains, verdict=verdict)
    args.records.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    (args.records / f"{args.name}-{stamp}.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(result["stdout"], end="")
    print(result["stderr"], end="", file=sys.stderr)
    print(f"{args.name}: {verdict}")
    return 77 if skipped else 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
