"""Bounded C16 test runner; expected failures must be clean checker rejections."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process  # noqa: E402


def run(command: list[str], timeout: float, expect_failure: str, allow_skip: bool) -> int:
    result = run_process(command, timeout)
    clean = not (result["timeout"] or result["error"] or result["cleanup_error"])
    combined = result["stdout"] + "\n" + result["stderr"]
    skipped = clean and allow_skip and result["exit_code"] == 77
    expected_bad = clean and result["exit_code"] == 1 and expect_failure and expect_failure in combined
    passed = clean and result["exit_code"] == 0 and not expect_failure

    print(result["stdout"], end="")
    print(result["stderr"], end="", file=sys.stderr)
    if result["timeout"]:
        print("run_check: FAIL timeout", file=sys.stderr)
    if result["error"]:
        print(f"run_check: FAIL {result['error']}", file=sys.stderr)
    if result["cleanup_error"]:
        print(f"run_check: FAIL cleanup {result['cleanup_error']}", file=sys.stderr)
    if skipped:
        print("run_check: SKIP")
        return 77
    if expected_bad or passed:
        print("run_check: PASS")
        return 0
    print(f"run_check: FAIL exit={result['exit_code']}", file=sys.stderr)
    return 1


def self_check() -> int:
    import tempfile

    here = Path(__file__).resolve()
    assert run([sys.executable, "-c", "print('ok')"], 5, "", False) == 0
    assert run([sys.executable, "-c", "import sys; sys.stdout.reconfigure(encoding='utf-8'); print(chr(0xfffd))"], 5, "", False) == 0
    assert run([sys.executable, "-c", "import sys; print('check failed: x'); sys.exit(1)"], 5,
               "check failed: x", False) == 0
    assert run([sys.executable, "-c", "import sys; print('SKIP: optional'); sys.exit(77)"], 5,
               "", True) == 77
    assert run([sys.executable, "-c", "import sys; print('SKIP: optional'); sys.exit(77)"], 5,
               "", False) == 1
    assert run([sys.executable, "-c", "import os, sys; sys.stderr.write('check failed: crash\\n'); os.abort()"], 5,
               "check failed: crash", False) == 1
    assert run([sys.executable, "-c", "import time; time.sleep(2)"], 0.2, "", False) == 1
    with tempfile.TemporaryDirectory(prefix="c16-run-check-") as temp:
        assert run([str(Path(temp) / "missing-program.exe")], 5, "", False) == 1
    assert here.exists()
    print("run_check self-check PASS")
    return 0


def main() -> int:
    # Compiler output may already contain replacement characters after decoding.
    # A legacy Windows console codec must not turn a completed check into a print exception.
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--expect-failure", default="")
    parser.add_argument("--allow-skip", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("a command is required after --")
    return run(command, args.timeout, args.expect_failure, args.allow_skip)


if __name__ == "__main__":
    raise SystemExit(main())
