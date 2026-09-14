"""Run the same independent byte oracle against one selected backend."""
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from run_check import run_process


def main() -> int:
    executable, backend = sys.argv[1:3]
    command = [executable, "--backend", backend]
    if backend == "plugin":
        command += ["--plugin", sys.argv[3]]
    table = bytes.maketrans(b"abcdefghijklmnopqrstuvwxyz", b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    for payload in (b"", b"a\x00z\xffA", bytes(range(256)), b"aBc\x00\xff" * 73):
        result = run_process(command + ["--hex", payload.hex()], 15)
        if result["status"] != "PASS" or result["stdout"].strip() != payload.translate(table).hex():
            print("check failed: backend byte oracle", result)
            return 1
    for invalid in ("0", "gg", "-1"):
        result = run_process(command + ["--hex", invalid], 15)
        if result["timeout"] or result["error"] or result["cleanup_error"] or result["exit_code"] != 2:
            print("check failed: malformed input rejection", result)
            return 1
    print(f"P1 {backend}: PASS (other backends not implied)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
