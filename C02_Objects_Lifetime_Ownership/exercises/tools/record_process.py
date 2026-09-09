"""Record one bounded command and check its declared outcome (Python 3.10+)."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if sys.platform == "win32":
    import ctypes
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.GetErrorMode.argtypes = []
    kernel32.GetErrorMode.restype = ctypes.c_uint
    kernel32.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel32.SetErrorMode.restype = ctypes.c_uint
    # Inherited only by this recorder's children; keep loader/crash failures in logs.
    kernel32.SetErrorMode(kernel32.GetErrorMode() | 0x0001 | 0x0002 | 0x8000)
sys.path.insert(0, str(Path(__file__).resolve().parents[3] /
                       "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=180)
    parser.add_argument("--expect-exit", type=int, default=0)
    parser.add_argument("--contains", action="append", default=[])
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("a command is required after --")
    if args.output.exists():
        parser.error("output already exists; choose a new evidence filename")
    result = run_process(command, args.timeout)
    result["cwd"] = str(Path.cwd())
    if sys.platform == "win32":
        result["inherited_windows_error_mode"] = kernel32.GetErrorMode()
    text = result["stdout"] + "\n" + result["stderr"]
    result["expected_exit"] = args.expect_exit
    result["required_text"] = args.contains
    result["verdict"] = "PASS" if (
        result["exit_code"] == args.expect_exit
        and not result["timeout"] and not result["error"] and not result["cleanup_error"]
        and all(fragment in text for fragment in args.contains)
    ) else "FAIL"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write("\n")
    print(f"{result['verdict']}: {args.output} (exit={result['exit_code']})")
    if result["verdict"] != "PASS":
        print(text[-6000:])
    return 0 if result["verdict"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
