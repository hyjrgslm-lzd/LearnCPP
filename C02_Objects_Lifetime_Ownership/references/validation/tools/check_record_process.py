"""Run the recorder's success, false-positive, timeout and preservation checks."""
from pathlib import Path
import argparse
import hashlib
import json
import sys

sys.dont_write_bytecode = True
CORE = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(CORE / "exercises/tools"))
from record_process import run_process


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = parser.parse_args().output
    if output.exists():
        parser.error("use a new output directory")
    output.mkdir(parents=True)
    recorder = CORE / "exercises/tools/record_process.py"
    cases = [
        ("success", [], "print('C02_OK')", 0, "PASS", False),
        ("expected-failure", ["--expect-exit", "7", "--contains", "C02_EXPECTED"],
         "import sys; print('C02_EXPECTED'); sys.exit(7)", 0, "PASS", False),
        ("wrong-marker", ["--contains", "MISSING"], "print('C02_OK')", 1, "FAIL", False),
        ("unicode-failure", ["--contains", "MISSING"],
         "import sys; sys.stdout.reconfigure(encoding='utf-8'); print('\\ufffd中文')", 1, "FAIL", False),
        ("timeout", ["--timeout", "0.2"], "import time; time.sleep(2)", 1, "FAIL", True),
    ]
    if sys.platform == "win32":
        cases.append(("inherited-error-mode", ["--contains", "C02_ERROR_MODE_OK"],
                      "import ctypes; m=ctypes.windll.kernel32.GetErrorMode(); "
                      "print('C02_ERROR_MODE_OK' if m & 0x8003 == 0x8003 else 'BAD_MODE')",
                      0, "PASS", False))
    for name, options, code, expected_exit, verdict, timed_out in cases:
        evidence = output / f"{name}.json"
        result = run_process([sys.executable, str(recorder), "--output", str(evidence),
                              *options, "--", sys.executable, "-c", code], 15)
        assert result["exit_code"] == expected_exit, result
        assert not result["timeout"] and not result["cleanup_error"], result
        assert "Traceback" not in result["stderr"], result
        payload = json.loads(evidence.read_text(encoding="utf-8"))
        assert payload["verdict"] == verdict and payload["timeout"] == timed_out, payload
    protected = output / "success.json"
    before = hashlib.sha256(protected.read_bytes()).digest()
    result = run_process([sys.executable, str(recorder), "--output", str(protected),
                          "--", sys.executable, "-c", "print('overwrite')"], 15)
    assert result["exit_code"] == 2 and "already exists" in result["stderr"], result
    assert hashlib.sha256(protected.read_bytes()).digest() == before
    (output / "summary.json").write_text(json.dumps({
        "verdict": "PASS", "cases": [case[0] for case in cases] + ["preserve-existing-output"],
        "python": sys.version, "recorder_sha256": hashlib.sha256(recorder.read_bytes()).hexdigest(),
    }, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(cases) + 1} recorder contracts")


if __name__ == "__main__":
    main()
