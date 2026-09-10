"""Verify C07 Student-only wiring and safe placeholder rejection."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

TOOLS = Path(__file__).resolve().parent
REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(TOOLS))
from run_test import supervise


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def target_file(build: Path, config: str, name: str) -> Path:
    reply = build / ".cmake/api/v1/reply"
    indexes = sorted(reply.glob("index-*.json"))
    if not indexes:
        raise ValueError("missing CMake file-api index")
    index = read_json(indexes[-1])
    refs = [entry for entry in index["objects"] if entry["kind"] == "codemodel" and entry["version"]["major"] == 2]
    if not refs:
        raise ValueError("missing codemodel-v2 reply")
    model = read_json(reply / refs[0]["jsonFile"])
    matches = [item for cfg in model["configurations"] if cfg["name"] == config
               for item in cfg["targets"] if read_json(reply / item["jsonFile"])["name"] == name]
    if len(matches) != 1:
        raise ValueError(f"target {name} absent or ambiguous in {config}")
    target = read_json(reply / matches[0]["jsonFile"])
    artifacts = target.get("artifacts", [])
    if not artifacts:
        raise ValueError(f"target {name} has no executable artifact")
    artifact = Path(artifacts[0]["path"])
    return artifact if artifact.is_absolute() else (build / artifact).resolve()


def same_path(left: str | Path, right: str | Path) -> bool:
    return os.path.normcase(str(Path(left).resolve())) == os.path.normcase(str(Path(right).resolve()))


def subject_argvs_from_ctest(data: dict) -> dict[str, list[str]]:
    subjects: dict[str, list[str]] = {}
    for test in data.get("tests", []):
        name = test.get("name")
        command = test.get("command", [])
        if not name or not isinstance(command, list):
            continue
        try:
            split = command.index("--")
        except ValueError:
            continue
        if not any(Path(str(part)).name == "run_test.py" for part in command[:split]):
            continue
        subject = [str(part) for part in command[split + 1:]]
        if subject:
            subjects[str(name)] = subject
    return subjects


def registered_student_argvs(build: Path, config: str, timeout: float = 20) -> dict[str, list[str]]:
    command = ["ctest", "--test-dir", str(build), "-C", config, "--show-only=json-v1"]
    result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8",
                            errors="replace", timeout=timeout)
    if result.returncode != 0:
        raise ValueError(f"ctest --show-only=json-v1 failed: {result.stderr or result.stdout}")
    return subject_argvs_from_ctest(json.loads(result.stdout))


def verify(build: Path, config: str, trace: Path, output: Path, timeout: float) -> dict:
    targets_file = build / f"student-targets-{config}.txt"
    targets = [line.strip() for line in targets_file.read_text(encoding="utf-8").splitlines() if line.strip()]
    if not targets:
        return {"verdict": "FAIL", "failures": [f"no student targets listed in {targets_file}"]}
    with tempfile.TemporaryDirectory(prefix="c07-student-audit-") as tmp:
        audit_out = Path(tmp) / "audit.json"
        command = [sys.executable, str(REPO / "C02_Objects_Lifetime_Ownership/exercises/tools/audit_student.py"),
                   "--build", str(build), "--config", config, "--trace", str(trace), "--output", str(audit_out)]
        for target in targets:
            command.extend(["--expect-target", target])
        audit_run = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
        audit = read_json(audit_out) if audit_out.exists() else {
            "verdict": "FAIL",
            "failures": ["audit_student.py did not write output"],
            "stdout": audit_run.stdout,
            "stderr": audit_run.stderr,
        }
    failures = []
    if audit_run.returncode != 0 or audit.get("verdict") != "PASS":
        failures.append("audit_student.py failed")
    registered = registered_student_argvs(build, config)
    runs = []
    for name in targets:
        exe = target_file(build, config, name)
        subject = registered.get(name)
        if subject is None:
            failures.append(f"{name} has no registered CTest student command")
            runs.append({"target": name, "artifact": str(exe), "controlled_rejection": False,
                         "error": "missing registered CTest command"})
            continue
        if not same_path(subject[0], exe):
            failures.append(f"{name} CTest command does not start with codemodel artifact")
            runs.append({"target": name, "artifact": str(exe), "registered_command": subject,
                         "controlled_rejection": False, "error": "registered executable mismatch"})
            continue
        result = supervise(subject, timeout)
        text = result["stdout"] + "\n" + result["stderr"]
        ok = (result["exit_code"] == 1 and not result["timeout"] and not result["error"]
              and not result["cleanup_error"] and "check failed" in text)
        if not ok:
            failures.append(f"{name} was not a controlled student rejection")
        runs.append({"target": name, "artifact": str(exe), "registered_command": subject,
                     "controlled_rejection": ok, **result})
    return {
        "verdict": "FAIL" if failures else "PASS",
        "failures": failures + audit.get("failures", []),
        "build": str(build.resolve()),
        "config": config,
        "targets": targets,
        "audit_student": audit,
        "student_runs": runs,
        "scope": "Student-only wiring plus direct placeholder rejection; not plagiarism proof",
    }


def self_check() -> int:
    with tempfile.TemporaryDirectory(prefix="c07-verify-students-") as d:
        root = Path(d)
        exe = root / "student.exe"
        module = root / "fixture.dll"
        exe.write_text("", encoding="utf-8")
        module.write_text("", encoding="utf-8")
        data = {"tests": [{"name": "L09_dynamic_loading_student",
                           "command": ["python", "run_test.py", "--name", "L09_dynamic_loading_student",
                                       "--", str(exe), str(module)]}]}
        subjects = subject_argvs_from_ctest(data)
        assert subjects["L09_dynamic_loading_student"] == [str(exe), str(module)]
        assert same_path(subjects["L09_dynamic_loading_student"][0], exe)
        assert "missing_student" not in subjects
        wrong = {"tests": [{"name": "L09_dynamic_loading_student",
                            "command": ["python", "run_test.py", "--", str(module), str(exe)]}]}
        assert not same_path(subject_argvs_from_ctest(wrong)["L09_dynamic_loading_student"][0], exe)
        script = ("import pathlib, tempfile; "
                  "pathlib.Path(tempfile.gettempdir(), 'left-by-child.txt').write_text('x'); "
                  "print('check failed: todo'); raise SystemExit(1)")
        record = supervise([sys.executable, "-c", script], 5)
        assert record["exit_code"] == 1 and "check failed" in record["stdout"]
        assert record["temporary_directory_removed"] is True
        assert not Path(record["temporary_directory"]).exists()
    print("verify_students self-check PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=False)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--trace", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--timeout", type=float, default=25)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        return self_check()
    for field in ("build", "trace", "output"):
        if getattr(args, field) is None:
            parser.error(f"--{field} is required")
    if args.output.exists():
        parser.error("choose a new output path")
    try:
        result = verify(args.build.resolve(), args.config, args.trace.resolve(), args.output, args.timeout)
    except Exception as error:
        result = {"verdict": "FAIL", "failures": [str(error)]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(f"{result['verdict']}: {args.output}")
    for failure in result.get("failures", []):
        print(failure)
    return int(result["verdict"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
