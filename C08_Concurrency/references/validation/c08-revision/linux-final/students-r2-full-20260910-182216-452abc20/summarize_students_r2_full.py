from __future__ import annotations

import json
from pathlib import Path

ROOT = Path.cwd()
RUN_ID = "students-r2-full-20260910-182216-452abc20"
EVIDENCE = ROOT / "C08_Concurrency" / "references" / "validation" / "c08-revision" / "linux-final" / RUN_ID


def main() -> int:
    stdout = (EVIDENCE / "verify-students-r2-full-ninja.stdout.txt").read_text(encoding="utf-8", errors="ignore")
    stderr = (EVIDENCE / "verify-students-r2-full-ninja.stderr.txt").read_text(encoding="utf-8", errors="ignore")
    lines = [line for line in stdout.splitlines() if line.startswith("{")]
    cases = [json.loads(line) for line in lines if '"target"' in line]
    tail = json.loads(lines[-1])
    good = [case for case in cases if case["mode"] == "good"]
    leaks = []
    missing = []
    for case in good:
        deps = "\n".join(case.get("student_dependency_trace", []))
        if case.get("dependency_trace_count", 0) <= 0:
            missing.append(case["target"])
        if "reference.hpp" in deps.lower() or "solution.cpp" in deps.lower() or "/validation/" in deps.lower():
            leaks.append(case["target"])
    compile_samples = (EVIDENCE / "compile-command-samples.txt").read_text(encoding="utf-8", errors="ignore")
    summary = {
        "runId": RUN_ID,
        "command": json.loads((EVIDENCE / "verify-students-r2-full-ninja.command.json").read_text(encoding="utf-8")),
        "snapshot": json.loads((EVIDENCE / "snapshot-summary.json").read_text(encoding="utf-8")),
        "scriptSummary": tail,
        "completedCases": len(cases),
        "modeCounts": {mode: sum(1 for case in cases if case["mode"] == mode) for mode in ("initial", "good", "bad")},
        "goodCases": len(good),
        "goodCasesWithDependencyTrace": sum(1 for case in good if case.get("dependency_trace_count", 0) > 0),
        "goodDependencyTraceMin": min(case.get("dependency_trace_count", 0) for case in good),
        "goodDependencyTraceMax": max(case.get("dependency_trace_count", 0) for case in good),
        "goodCasesMissingDependencyTrace": missing,
        "referenceLeakTargets": leaks,
        "compileCommandSamplesHaveO3AndDNDEBUG": "has_O3=True" in compile_samples and "has_DNDEBUG=True" in compile_samples,
        "stderrBytes": len(stderr.encode("utf-8")),
        "evidenceLogFiles": len(list(EVIDENCE.rglob("*.log"))),
    }
    (EVIDENCE / "students-r2-full-summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
