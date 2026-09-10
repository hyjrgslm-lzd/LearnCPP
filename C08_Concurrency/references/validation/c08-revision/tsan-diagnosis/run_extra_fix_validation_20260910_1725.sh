#!/usr/bin/env bash
set -u

src="/root/learncpp-c08-builds/tsan-fix-src-20260910-1715"
build_asan="/root/learncpp-c08-builds/tsan-fix-asan-20260910-1725"
report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/fix-run-20260910-1725"

mkdir -p "$report"
rm -rf "$build_asan"

cmake -S "$src/C08_Concurrency/exercises" -B "$build_asan" -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON \
    -DCONCURRENCY_STUDY_TEST_STARTERS=OFF \
    -DCONCURRENCY_STUDY_SANITIZER=address \
    > "$report/linux-asan-configure.txt" 2>&1
printf '%s\n' "$?" > "$report/status-linux-asan-configure.txt"

cmake --build "$build_asan" --target \
    B3_call_once_reference \
    F3_seqcst_fence_reference \
    runtime_scheduling_test \
    runtime_scheduling_allocation_test \
    > "$report/linux-asan-build-targets.txt" 2>&1
printf '%s\n' "$?" > "$report/status-linux-asan-build-targets.txt"

ctest --test-dir "$build_asan" -R '^(B3_call_once_reference|F3_seqcst_fence_reference|runtime_scheduling_test|runtime_scheduling_allocation_test)$' --output-on-failure \
    > "$report/linux-asan-ctest-targets.txt" 2>&1
printf '%s\n' "$?" > "$report/status-linux-asan-ctest-targets.txt"

for f in "$report"/status-*.txt; do
    printf '%s=' "$(basename "$f")"
    cat "$f"
done | sort > "$report/status-summary.tmp"
mv "$report/status-summary.tmp" "$report/status-summary.txt"

python3 - <<PY
import json
from pathlib import Path
report = Path("$report")
statuses = {p.name: p.read_text(encoding="utf-8", errors="replace").strip()
            for p in sorted(report.glob("status-*.txt"))
            if p.name != "status-summary.txt"}
(report / "status-summary.json").write_text(json.dumps(statuses, indent=2, sort_keys=True) + "\\n", encoding="utf-8")
PY

cat "$report/status-summary.txt"
