"""Check the outcome classifier with real bounded child processes."""
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from run_check import run


def main() -> int:
    cases = [
        ([sys.executable, "-c", "print('ok')"], 5, "", False, 0),
        ([sys.executable, "-c", "print('check failed: capacity'); raise SystemExit(1)"], 5, "check failed: capacity", False, 0),
        ([sys.executable, "-c", "print('check failed: other'); raise SystemExit(1)"], 5, "check failed: capacity", False, 1),
        ([sys.executable, "-c", "print('check failed: capacity')"], 5, "check failed: capacity", False, 1),
        ([sys.executable, "-c", "print('check failed: capacity'); raise SystemExit(2)"], 5, "check failed: capacity", False, 1),
        ([sys.executable, "-c", "raise SystemExit(77)"], 5, "", True, 77),
        ([sys.executable, "-c", "raise SystemExit(77)"], 5, "", False, 1),
        ([sys.executable, "-c", "import time; time.sleep(3)"], 0.1, "", False, 1),
        ([str(Path(__file__).parent / "nonexistent-c18-executable")], 5, "", False, 1),
    ]
    for command, timeout, diagnostic, skip, expected in cases:
        actual = run(command, timeout, diagnostic, skip)
        if actual != expected:
            print(f"check failed: runner expected {expected}, got {actual}")
            return 1
    print("runner classification PASS (including deliberately rejected child results)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
