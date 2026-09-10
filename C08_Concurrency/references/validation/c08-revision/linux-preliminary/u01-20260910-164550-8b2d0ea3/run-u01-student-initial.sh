#!/usr/bin/env bash
set -u

source_root="/root/learncpp-c08-src/u01-20260910-164550-8b2d0ea3/C08_Concurrency/exercises/U01_async_logging"
build_root="/root/learncpp-c08-builds/u01-20260910-164550-8b2d0ea3"
evidence="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/u01-20260910-164550-8b2d0ea3"
fmt_source="/root/learncpp-c08-deps/fmt-12.1.0"
spdlog_source="/root/learncpp-c08-deps/spdlog-1.17.0"

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

run_one() {
    local name="$1"
    local compiler="$2"
    local sanitizer="$3"
    local build_type="$4"
    local build_dir="${build_root}/${name}-student"
    mkdir -p "${build_dir}"
    run_step "${name}-student-configure" cmake -S "${source_root}" -B "${build_dir}" -G Ninja \
        -DCMAKE_CXX_COMPILER="${compiler}" \
        -DCMAKE_BUILD_TYPE="${build_type}" \
        -DBUILD_TESTING=ON \
        -DCONCURRENCY_STUDY_ENABLE_SPDLOG=ON \
        -DCONCURRENCY_STUDY_BUILD_REFERENCE=OFF \
        -DCONCURRENCY_STUDY_TEST_STARTERS=ON \
        -DCONCURRENCY_STUDY_BUILD_BENCHMARKS=OFF \
        -DCONCURRENCY_STUDY_FETCH_DEPS=OFF \
        -DCONCURRENCY_STUDY_SANITIZER="${sanitizer}" \
        -DCONCURRENCY_STUDY_FMT_SOURCE_DIR="${fmt_source}" \
        -DCONCURRENCY_STUDY_SPDLOG_SOURCE_DIR="${spdlog_source}" || return
    run_step "${name}-student-build" cmake --build "${build_dir}" --parallel 4 || return
    run_step "${name}-student-ctest-list" ctest --test-dir "${build_dir}" --show-only=json-v1
    ctest --test-dir "${build_dir}" --show-only=json-v1 > "${evidence}/${name}-student-ctest-list.json"
    run_step "${name}-student-ctest-expected-fail" ctest --test-dir "${build_dir}" -C "${build_type}" \
        -R '^U01_async_logging_student$' --output-on-failure --output-junit "${evidence}/${name}-student-expected-fail-junit.xml"
}

run_one release g++ none Release
run_one asan clang++-18 address RelWithDebInfo
run_one tsan clang++-18 thread RelWithDebInfo
