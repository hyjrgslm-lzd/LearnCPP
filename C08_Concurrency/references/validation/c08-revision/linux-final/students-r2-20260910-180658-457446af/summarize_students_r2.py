from __future__ import annotations

import json
from pathlib import Path

ROOT = Path.cwd()
RUN_ID = "students-r2-20260910-180658-457446af"
EVIDENCE = ROOT / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / RUN_ID


def main() -> int:
    stdout = (EVIDENCE / "verify-students-r2-ninja.stdout.txt").read_text(encoding="utf-8", errors="ignore")
    stderr = (EVIDENCE / "verify-students-r2-ninja.stderr.txt").read_text(encoding="utf-8", errors="ignore")
    cases = [json.loads(line) for line in stdout.splitlines() if line.startswith("{") and '"target"' in line]
    good_cases = [case for case in cases if case["mode"] == "good"]
    leaks = []
    missing_trace = []
    for case in good_cases:
        deps = "\n".join(case.get("student_dependency_trace", []))
        if "reference.hpp" in deps.lower() or "solution.cpp" in deps.lower() or "/validation/" in deps.lower():
            leaks.append(case["target"])
        if case.get("dependency_trace_count", 0) <= 0:
            missing_trace.append(case["target"])
    summary = {
        "runId": RUN_ID,
        "command": json.loads((EVIDENCE / "verify-students-r2-ninja.command.json").read_text(encoding="utf-8")),
        "snapshot": json.loads((EVIDENCE / "snapshot-summary.json").read_text(encoding="utf-8")),
        "completedCases": len(cases),
        "completedTargets": sorted({case["target"] for case in cases}),
        "modeCounts": {mode: sum(1 for case in cases if case["mode"] == mode) for mode in ("initial", "good", "bad")},
        "goodCases": len(good_cases),
        "goodCasesWithDependencyTrace": sum(1 for case in good_cases if case.get("dependency_trace_count", 0) > 0),
        "goodCasesMissingDependencyTrace": missing_trace,
        "referenceLeakTargets": leaks,
        "sampleReleaseFlagsObserved": "-O3 -DNDEBUG" in stderr or "-O3 -DNDEBUG" in stdout,
        "blockingFailure": "I3_rcu initial compile failed: ../../topics/reclamation/experiment_support.hpp not found",
        "stderrTail": stderr[-4000:],
        "evidenceLogFiles": len(list(EVIDENCE.rglob("*.log"))),
    }
    (EVIDENCE / "students-r2-summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
