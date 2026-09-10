#!/usr/bin/env bash
set -u

src="/root/learncpp-c08-builds/tsan-fix-src-20260910-1715"
build_plain="/root/learncpp-c08-builds/tsan-fix-plain-20260910-1715"
build_tsan="/root/learncpp-c08-builds/tsan-fix-tsan-20260910-1715"
report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/fix-run-20260910-1715"

mkdir -p "$report"
rm -rf "$src" "$build_plain" "$build_tsan"
mkdir -p "$src"
(
    cd /mnt/f/CPPTrain/LearnCPP || exit 2
    tar --exclude='C08_Concurrency/exercises/build*' -cf - C08_Concurrency/exercises
) | (
    cd "$src" || exit 2
    tar -xf -
)

{
    printf 'src=%s\n' "$src"
    printf 'build_plain=%s\n' "$build_plain"
    printf 'build_tsan=%s\n' "$build_tsan"
    clang++-18 --version | head -n 1
    g++ --version | head -n 1
    cmake --version | head -n 1
    ninja --version
} > "$report/environment.txt"

sha256sum \
    "$src/C08_Concurrency/exercises/B3_call_once/solution.cpp" \
    "$src/C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp" \
    "$src/C08_Concurrency/exercises/runtime_tests/scheduling_test.cpp" \
    "$src/C08_Concurrency/exercises/runtime_tests/scheduling_allocation_test.cpp" \
    > "$report/source-hashes.txt"

cmake -S "$src/C08_Concurrency/exercises" -B "$build_plain" -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON \
    -DCONCURRENCY_STUDY_TEST_STARTERS=OFF \
    -DCONCURRENCY_STUDY_SANITIZER=none \
    > "$report/plain-configure.txt" 2>&1
printf '%s\n' "$?" > "$report/status-plain-configure.txt"

cmake --build "$build_plain" --target \
    B3_call_once_reference \
    F3_seqcst_fence_reference \
    runtime_scheduling_test \
    runtime_scheduling_allocation_test \
    > "$report/plain-build-targets.txt" 2>&1
printf '%s\n' "$?" > "$report/status-plain-build-targets.txt"

ctest --test-dir "$build_plain" -R '^(B3_call_once_reference|F3_seqcst_fence_reference|runtime_scheduling_test|runtime_scheduling_allocation_test)$' --output-on-failure \
    > "$report/plain-ctest-targets.txt" 2>&1
printf '%s\n' "$?" > "$report/status-plain-ctest-targets.txt"

cmake -S "$src/C08_Concurrency/exercises" -B "$build_tsan" -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON \
    -DCONCURRENCY_STUDY_TEST_STARTERS=OFF \
    -DCONCURRENCY_STUDY_SANITIZER=thread \
    > "$report/tsan-configure.txt" 2>&1
printf '%s\n' "$?" > "$report/status-tsan-configure.txt"

cmake --build "$build_tsan" --target \
    B3_call_once_reference \
    F3_seqcst_fence_reference \
    runtime_scheduling_test \
    runtime_scheduling_allocation_test \
    > "$report/tsan-build-targets.txt" 2>&1
printf '%s\n' "$?" > "$report/status-tsan-build-targets.txt"

ctest --test-dir "$build_tsan" -R '^(B3_call_once_reference|F3_seqcst_fence_reference|runtime_scheduling_test|runtime_scheduling_allocation_test)$' --output-on-failure \
    > "$report/tsan-ctest-targets.txt" 2>&1
printf '%s\n' "$?" > "$report/status-tsan-ctest-targets.txt"

/usr/bin/timeout 15 "$build_tsan/B3_call_once/B3_call_once_reference" > "$report/tsan-direct-B3.txt" 2>&1
printf '%s\n' "$?" > "$report/status-tsan-direct-B3.txt"

/usr/bin/timeout 15 "$build_tsan/F3_seqcst_fence/F3_seqcst_fence_reference" > "$report/tsan-direct-F3.txt" 2>&1
printf '%s\n' "$?" > "$report/status-tsan-direct-F3.txt"

/usr/bin/timeout 15 "$build_tsan/runtime_tests/runtime_scheduling_allocation_test" > "$report/tsan-direct-scheduling-allocation.txt" 2>&1
printf '%s\n' "$?" > "$report/status-tsan-direct-scheduling-allocation.txt"

rm -f "$report/status-summary.txt" "$report/status-summary.json"
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
            if p.name not in {"status-summary.txt"}}
(report / "status-summary.json").write_text(json.dumps(statuses, indent=2, sort_keys=True) + "\\n", encoding="utf-8")
PY

cat "$report/status-summary.txt"
