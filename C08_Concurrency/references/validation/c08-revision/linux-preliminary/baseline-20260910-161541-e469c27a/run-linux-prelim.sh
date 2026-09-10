#!/usr/bin/env bash
set -u
src=/root/learncpp-c08-src/baseline-20260910-161541-e469c27a/C08_Concurrency/exercises
build_root=/root/learncpp-c08-builds/baseline-20260910-161541-e469c27a
evidence=/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-161541-e469c27a
mkdir -p "$build_root" "$evidence"

run_step() {
    local name="$1"
    shift
    echo "COMMAND: $*" > "$evidence/${name}.log"
    echo "START: $(date -Iseconds)" >> "$evidence/${name}.log"
    timeout 900s "$@" >> "$evidence/${name}.log" 2>&1
    local rc=$?
    echo "END: $(date -Iseconds)" >> "$evidence/${name}.log"
    echo "EXIT: $rc" >> "$evidence/${name}.log"
    echo "$name=$rc" | tee -a "$evidence/results.env"
    return $rc
}

: > "$evidence/results.env"
configs=(linux-core linux-debug linux-asan linux-tsan)
for cfg in "${configs[@]}"; do
    b="$build_root/$cfg"
    mkdir -p "$b"
    run_step "${cfg}-configure" cmake -S "$src" -B "$b" --preset "$cfg" -DCMAKE_BUILD_TYPE=$(case "$cfg" in linux-debug) echo Debug ;; linux-asan|linux-tsan) echo RelWithDebInfo ;; *) echo Release ;; esac) || continue
    run_step "${cfg}-build" cmake --build "$b" --parallel 4 || continue
    run_step "${cfg}-ctest-list" ctest --test-dir "$b" --show-only=json-v1 || true
    cp "$evidence/${cfg}-ctest-list.log" "$evidence/${cfg}-ctest-list.jsonish" 2>/dev/null || true
    run_step "${cfg}-ctest" ctest --test-dir "$b" -C $(case "$cfg" in linux-debug) echo Debug ;; linux-asan|linux-tsan) echo RelWithDebInfo ;; *) echo Release ;; esac) -E '^runtime_materials$' --output-on-failure --output-junit "$evidence/${cfg}-junit.xml"
done