#!/usr/bin/env bash
set -u

RUN_ID="m1-r2-final-20260910-181517-b97f497d"
SRC="/root/learncpp-c08-src/${RUN_ID}/C08_Concurrency/exercises"
SNAPSHOT="/root/learncpp-c08-src/${RUN_ID}"
BUILD_ROOT="/root/learncpp-c08-builds/${RUN_ID}"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
FMT_SRC="/root/learncpp-c08-deps/fmt-12.1.0"
SPDLOG_SRC="/root/learncpp-c08-deps/spdlog-1.17.0"
COMMANDS_JSONL="${EVIDENCE}/commands.jsonl"

mkdir -p "${BUILD_ROOT}" "${EVIDENCE}"
: > "${COMMANDS_JSONL}"

json_escape() { python3 -c 'import json,sys; print(json.dumps(sys.stdin.read())[1:-1])'; }

record_command() {
  local name="$1"; shift
  local timeout_seconds="$1"; shift
  local out="${EVIDENCE}/${name}.txt"
  local start end rc escaped_cmd
  start="$(date --iso-8601=seconds)"
  escaped_cmd="$(printf '%s ' "$@" | json_escape)"
  {
    echo "name=${name}"
    echo "start=${start}"
    echo "timeout_seconds=${timeout_seconds}"
    echo "command=$*"
    echo
  } > "${out}"
  timeout "${timeout_seconds}" "$@" >> "${out}" 2>&1
  rc=$?
  end="$(date --iso-8601=seconds)"
  {
    echo
    echo "exit_code=${rc}"
    echo "end=${end}"
  } >> "${out}"
  printf '{"name":"%s","start":"%s","end":"%s","timeout_seconds":%s,"exit_code":%s,"command":"%s","output":"%s"}\n' \
    "${name}" "${start}" "${end}" "${timeout_seconds}" "${rc}" "${escaped_cmd}" "${name}.txt" >> "${COMMANDS_JSONL}"
  return "${rc}"
}

record_allow_fail() { record_command "$@" || true; }

write_context() {
  {
    echo "run_id=${RUN_ID}"
    echo "snapshot=${SNAPSHOT}"
    echo "source=${SRC}"
    echo "build_root=${BUILD_ROOT}"
    echo
    uname -a
    cmake --version
    ninja --version
    g++ --version
    clang++-18 --version
    python3 --version
    git --version
    echo
    echo "fmt=$(git -C "${FMT_SRC}" rev-parse HEAD)"
    echo "spdlog=$(git -C "${SPDLOG_SRC}" rev-parse HEAD)"
  } > "${EVIDENCE}/toolchain-and-deps.txt" 2>&1
  (
    cd "${SNAPSHOT}" &&
    find . -type f -print0 | sort -z | xargs -0 sha256sum
  ) > "${EVIDENCE}/guest-source-manifest.sha256.txt"
  sha256sum "${EVIDENCE}/guest-source-manifest.sha256.txt" > "${EVIDENCE}/guest-source-manifest-file.sha256.txt"
  {
    echo "hidden_dirs:"
    find "${SNAPSHOT}" -type d -name '.*' -print
    echo "build_dirs:"
    find "${SNAPSHOT}" -type d \( -name 'build*' -o -name '_deps' -o -name 'CMakeFiles' \) -print
    echo "forbidden_files:"
    find "${SNAPSHOT}" -type f \( -name '*.exe' -o -name '*.dll' -o -name '*.pdb' -o -name '*.ilk' -o -name '*.obj' -o -name '*.o' -o -name '*.lib' -o -name '*.a' -o -name '*.so' -o -name '*.dylib' -o -name '*.zip' -o -name '*.7z' -o -name '*.tar' -o -name '*.gz' -o -name '*.xz' -o -name '*.bz2' -o -name '*.bin' -o -name '*.tmp' -o -name '*.log' -o -name 'CMakeCache.txt' -o -name 'compile_commands.json' \) -print
  } > "${EVIDENCE}/snapshot-leak-check.txt"
}

run_config() {
  local name="$1"
  local preset="$2"
  local build_type="$3"
  local sanitizer="$4"
  local build_dir="${BUILD_ROOT}/${name}"
  mkdir -p "${build_dir}"
  record_allow_fail "${name}-configure" 300 \
    cmake -S "${SRC}" -B "${build_dir}" --preset "${preset}" \
      -DCMAKE_BUILD_TYPE="${build_type}" \
      -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON \
      -DCONCURRENCY_STUDY_TEST_FAST_MATH=OFF \
      -DCONCURRENCY_STUDY_FETCH_DEPS=OFF \
      -DCONCURRENCY_STUDY_ENABLE_SPDLOG=ON \
      -DCONCURRENCY_STUDY_FMT_SOURCE_DIR="${FMT_SRC}" \
      -DCONCURRENCY_STUDY_SPDLOG_SOURCE_DIR="${SPDLOG_SRC}" \
      -DCONCURRENCY_STUDY_ENABLE_XSIMD=OFF \
      -DCONCURRENCY_STUDY_ENABLE_STDEXEC=OFF \
      -DCONCURRENCY_STUDY_ENABLE_CXX26=OFF \
      -DCONCURRENCY_STUDY_ENABLE_CXX29=OFF \
      -DCONCURRENCY_STUDY_BUILD_BENCHMARKS=OFF \
      -DCONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS=OFF \
      -DCONCURRENCY_STUDY_SANITIZER="${sanitizer}"
  record_allow_fail "${name}-build" 1200 cmake --build "${build_dir}" --parallel 4
  record_allow_fail "${name}-ctest-list" 120 ctest --test-dir "${build_dir}" -C "${build_type}" --show-only=json-v1
  mv "${EVIDENCE}/${name}-ctest-list.txt" "${EVIDENCE}/${name}-ctest-list-output.txt"
  ctest --test-dir "${build_dir}" -C "${build_type}" --show-only=json-v1 > "${EVIDENCE}/${name}-ctest-list.json" 2> "${EVIDENCE}/${name}-ctest-list-stderr.txt" || true
  record_allow_fail "${name}-ctest" 1200 ctest --test-dir "${build_dir}" -C "${build_type}" --output-on-failure --output-junit "${EVIDENCE}/${name}-junit.xml" --timeout 120
}

summarize() {
  python3 - <<'PY' > "${EVIDENCE}/m1-r2-matrix-summary.json"
import json
import pathlib
import xml.etree.ElementTree as ET

evidence = pathlib.Path("/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/m1-r2-final-20260910-181517-b97f497d")
commands = [json.loads(line) for line in (evidence / "commands.jsonl").read_text(encoding="utf-8").splitlines() if line.strip()]
configs = {}
for xml_path in sorted(evidence.glob("*-junit.xml")):
    name = xml_path.name.removesuffix("-junit.xml")
    root = ET.parse(xml_path).getroot()
    cases = root.findall(".//testcase")
    skipped = [c.get("name", "") for c in cases if c.find("skipped") is not None]
    failed = [c.get("name", "") for c in cases if c.find("failure") is not None or c.find("error") is not None]
    configs[name] = {"tests": len(cases), "skipped": len(skipped), "failed": len(failed), "skippedTests": skipped, "failedTests": failed}
registration = {}
for list_path in sorted(evidence.glob("*-ctest-list.json")):
    data = json.loads(list_path.read_text(encoding="utf-8"))
    names = [t.get("name", "") for t in data.get("tests", [])]
    registration[list_path.name] = {
        "testCount": len(names),
        "has_runtime_benchmark_tools": "runtime_benchmark_tools" in names,
        "has_runtime_materials": "runtime_materials" in names,
    }
print(json.dumps({"commands": commands, "configs": configs, "registration": registration}, indent=2, ensure_ascii=False))
PY
}

write_context
run_config "gcc-release" "linux-core" "Release" "none"
run_config "gcc-debug" "linux-debug" "Debug" "none"
run_config "clang-asan-ubsan" "linux-asan" "RelWithDebInfo" "address"
run_config "clang-tsan" "linux-tsan" "RelWithDebInfo" "thread"
summarize
