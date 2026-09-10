from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path.cwd()
RUN_ID = "student-portability-20260910-175508-13ddcee8"
EVIDENCE = ROOT / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / RUN_ID


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def command_meta(name: str) -> dict:
    return json.loads(read(EVIDENCE / f"{name}.command.json"))


def inspect_case(case: str, inspection_name: str, stdout_name: str, stderr_name: str, run_json: str) -> dict:
    text = read(EVIDENCE / inspection_name)
    command_match = re.search(r'"command": "([^"]+)"', text)
    command = command_match.group(1) if command_match else ""
    depfiles_section = ""
    if "\ndepfiles\n" in text and "\ncompile_commands\n" in text:
        depfiles_section = text.split("\ndepfiles\n", 1)[1].split("\ncompile_commands\n", 1)[0].strip()
    stdout = read(EVIDENCE / stdout_name)
    stderr = read(EVIDENCE / stderr_name)
    run = json.loads(read(EVIDENCE / run_json))
    return {
        "case": case,
        "inspection": inspection_name,
        "compileCommand": command,
        "hasO3": "-O3" in command,
        "hasDNDEBUG": "-DNDEBUG" in command,
        "hasCmakeBuildTypeReleaseFlag": "CMAKE_BUILD_TYPE=Release" in command,
        "depfilesListed": [line for line in depfiles_section.splitlines() if line.strip()],
        "topLevelStdoutBytes": len(stdout.encode("utf-8")),
        "topLevelStderrBytes": len(stderr.encode("utf-8")),
        "executableRunExitCode": run.get("exit_code"),
        "executableRunStatus": run.get("status"),
        "executableRunTimeout": run.get("timeout"),
        "runtimeError": next((line for line in stderr.splitlines() if line.startswith("RuntimeError: ")), ""),
    }


def main() -> int:
    snapshot = json.loads(read(EVIDENCE / "snapshot-summary.json"))
    summary = {
        "runId": RUN_ID,
        "snapshot": snapshot,
        "commands": [
            command_meta("verify-students-ninja-good"),
            command_meta("verify-students-ninja-cap3-good"),
        ],
        "cases": [
            inspect_case(
                "I2_hazard_pointer good",
                "i2-build-inspection.txt",
                "verify-students-ninja-good.stdout.txt",
                "verify-students-ninja-good.stderr.txt",
                "case-outputs/i2-good/run.json",
            ),
            inspect_case(
                "Capstone3_parallel_compute good",
                "cap3-build-inspection.txt",
                "verify-students-ninja-cap3-good.stdout.txt",
                "verify-students-ninja-cap3-good.stderr.txt",
                "case-outputs/cap3-good/run.json",
            ),
        ],
        "rootCause": [
            "configure_and_build passes --config Release to cmake --build but does not set CMAKE_BUILD_TYPE=Release during Ninja configure",
            "compile_commands for both Ninja cases lack -O3 and -DNDEBUG",
            "collect_dependency_trace has no POSIX include-output regex and no depfiles were present after the Ninja build, so good-case dependency audit fails with an empty trace",
        ],
        "evidenceLogFiles": len(list(EVIDENCE.rglob("*.log"))),
    }
    (EVIDENCE / "student-portability-summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
