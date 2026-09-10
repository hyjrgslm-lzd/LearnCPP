#!/usr/bin/env bash
set -u

source_root="/root/learncpp-c08-src/baseline-20260910-162842-fc6bb723/C08_Concurrency/exercises"
build_root="/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723"
evidence="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-162842-fc6bb723"
exclude_tests='^(runtime_materials|U01_async_logging|F03_native_facilities)'

mkdir -p "${build_root}" "${evidence}"
: > "${evidence}/results.env"

build_type_for() {
    case "$1" in
        linux-debug) echo Debug ;;
        linux-asan|linux-tsan) echo RelWithDebInfo ;;
        *) echo Release ;;
    esac
}

run_step() {
    local name="$1"
    shift
    echo "COMMAND: $*" > "${evidence}/${name}.txt"
    echo "START: $(date -Iseconds)" >> "${evidence}/${name}.txt"
    timeout 900s "$@" >> "${evidence}/${name}.txt" 2>&1
    local rc=$?
    echo "END: $(date -Iseconds)" >> "${evidence}/${name}.txt"
    echo "EXIT: ${rc}" >> "${evidence}/${name}.txt"
    echo "${name}=${rc}" | tee -a "${evidence}/results.env"
    return "${rc}"
}

for preset in linux-core linux-debug linux-asan linux-tsan; do
    build_dir="${build_root}/${preset}"
    build_type="$(build_type_for "${preset}")"
    mkdir -p "${build_dir}"
    run_step "${preset}-configure" cmake -S "${source_root}" -B "${build_dir}" --preset "${preset}" -DCMAKE_BUILD_TYPE="${build_type}" || continue
    run_step "${preset}-build" cmake --build "${build_dir}" --parallel 4 || continue
    run_step "${preset}-ctest-list" ctest --test-dir "${build_dir}" --show-only=json-v1
    run_step "${preset}-ctest" ctest --test-dir "${build_dir}" -C "${build_type}" -E "${exclude_tests}" --output-on-failure --output-junit "${evidence}/${preset}-junit.xml"
done
