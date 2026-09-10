"""Calibrate verdict handling against success, exact rejection and timeout."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

runner = Path(__file__).with_name("run_test.py")
with tempfile.TemporaryDirectory(prefix="c07-runner-") as directory:
    root = Path(directory)
    cases = [("success", "print('ok')", 0, "ok", 0, 3),
             ("rejected", "print('contract rejected'); raise SystemExit(1)", 1, "contract rejected", 0, 3),
             ("wrong-diagnostic", "print('different'); raise SystemExit(1)", 1, "contract rejected", 1, 3),
             ("fixture-cleanup", "import os; from pathlib import Path; (Path(os.environ['TMPDIR'])/'left-behind').write_text('fixture'); print('cleanup control'); raise SystemExit(1)", 1, "cleanup control", 0, 3),
             ("timeout-is-failure", "import time; time.sleep(10)", 1, "", 1, 0.2)]
    for name, code, expected, fragment, verdict, timeout in cases:
        run = subprocess.run([sys.executable, str(runner), "--name", name, "--records", str(root),
                              "--timeout", str(timeout), "--expect-exit", str(expected),
                              "--contains", fragment, "--", sys.executable, "-c", code],
                             capture_output=True, text=True, timeout=25)
        if run.returncode != verdict:
            raise RuntimeError(f"runner control {name}: {run.stdout} {run.stderr}")
    timeout_record = json.loads(next(root.glob("timeout-is-failure-*.json")).read_text(encoding="utf-8"))
    if not timeout_record["timeout"] or timeout_record["verdict"] != "FAIL" or timeout_record["cleanup_error"]:
        detail = {key: timeout_record.get(key) for key in
                  ("timeout", "verdict", "exit_code", "error", "cleanup_error", "temporary_directory_removed")}
        raise RuntimeError("timeout control did not prove bounded cleanup: " + json.dumps(detail, ensure_ascii=False))
    cleanup_record = json.loads(next(root.glob("fixture-cleanup-*.json")).read_text(encoding="utf-8"))
    if not cleanup_record["temporary_directory_removed"] or Path(cleanup_record["temporary_directory"]).exists():
        raise RuntimeError("failed subject left its fixture directory behind")
print("runner controls: positive, exact rejection, diagnostic mismatch, timeout and fixture cleanup distinguished")
