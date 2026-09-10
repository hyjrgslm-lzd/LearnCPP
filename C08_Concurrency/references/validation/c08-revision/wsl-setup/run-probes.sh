#!/usr/bin/env bash
set -euo pipefail

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="/tmp/c08-wsl-probe"
mkdir -p "${build_dir}"

g++ -std=c++23 -pthread "${src_dir}/cxx23_probe.cpp" -o "${build_dir}/gcc_cxx23"
timeout 10s "${build_dir}/gcc_cxx23"

clang++-18 -std=c++23 -pthread "${src_dir}/cxx23_probe.cpp" -o "${build_dir}/clang_cxx23"
timeout 10s "${build_dir}/clang_cxx23"

clang++-18 -std=c++23 -pthread -fsanitize=thread "${src_dir}/tsan_clean.cpp" -o "${build_dir}/tsan_clean"
set +e
timeout 20s "${build_dir}/tsan_clean" >"${build_dir}/tsan_clean.out" 2>"${build_dir}/tsan_clean.err"
clean_rc=$?
set -e
echo "tsan_clean_rc=${clean_rc}"
if [[ -s "${build_dir}/tsan_clean.err" ]]; then
    head -n 20 "${build_dir}/tsan_clean.err"
fi

clang++-18 -std=c++23 -pthread -fsanitize=thread "${src_dir}/tsan_race.cpp" -o "${build_dir}/tsan_race"
set +e
timeout 20s "${build_dir}/tsan_race" >"${build_dir}/tsan_race.out" 2>"${build_dir}/tsan_race.err"
race_rc=$?
set -e
echo "tsan_race_rc=${race_rc}"
if grep -q "WARNING: ThreadSanitizer: data race" "${build_dir}/tsan_race.err"; then
    echo "tsan_race_warning=detected"
else
    echo "tsan_race_warning=missing"
    head -n 40 "${build_dir}/tsan_race.err"
fi
