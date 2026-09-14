"""Only a failed capability probe may skip; subject failures remain failures."""
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from run_check import run_process


def main() -> int:
    subject = Path(__file__).with_name("subject.py")
    probe = run_process([sys.executable, str(subject), sys.argv[1], "--probe"], 10)
    print(probe["stdout"], end="")
    if probe["timeout"] or probe["error"] or probe["cleanup_error"]:
        print(probe, file=sys.stderr)
        return 1
    if probe["exit_code"] == 77:
        return 77
    if probe["exit_code"] != 0:
        print(probe["stderr"], file=sys.stderr)
        return 1
    result = run_process([sys.executable, str(subject)] + sys.argv[1:], 30)
    print(result["stdout"], end="")
    print(result["stderr"], end="", file=sys.stderr)
    if result["status"] != "PASS":
        print(result, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
