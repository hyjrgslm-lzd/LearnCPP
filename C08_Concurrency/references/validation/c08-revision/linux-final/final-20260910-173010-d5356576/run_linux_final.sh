#!/usr/bin/env bash
set -u

RUN_ID="final-20260910-173010-d5356576"
SRC="/root/learncpp-c08-src/${RUN_ID}/C08_Concurrency/exercises"
SNAPSHOT="/root/learncpp-c08-src/${RUN_ID}"
BUILD_ROOT="/root/learncpp-c08-builds/${RUN_ID}"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
FMT_SRC="/root/learncpp-c08-deps/fmt-12.1.0"
SPDLOG_SRC="/root/learncpp-c08-deps/spdlog-1.17.0"
COMMANDS_JSONL="${EVIDENCE}/commands.jsonl"

mkdir -p "${BUILD_ROOT}" "${EVIDENCE}"
: > "${COMMANDS_JSONL}"

json_escape() {
  python3 -c 'import json,sys; print(json.dumps(sys.stdin.read())[1:-1])'
}

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

record_command_allow_fail() {
  record_command "$@" || true
}

write_toolchain() {
  {
    echo "run_id=${RUN_ID}"
    echo "snapshot=${SNAPSHOT}"
    echo "source=${SRC}"
    echo "build_root=${BUILD_ROOT}"
    echo "evidence=${EVIDENCE}"
    echo
    uname -a
    echo
    wslinfo --wsl-version 2>/dev/null || true
    echo
    cmake --version
    echo
    ninja --version
    echo
    g++ --version
    echo
    clang++-18 --version
    echo
    python3 --version
    echo
    git --version
    echo
    dpkg-query -W g++ clang-18 cmake ninja-build python3 git libclang-rt-18-dev 2>/dev/null || true
    echo
    echo "fmt:"
    git -C "${FMT_SRC}" rev-parse HEAD
    git -C "${FMT_SRC}" status --short
    echo
    echo "spdlog:"
    git -C "${SPDLOG_SRC}" rev-parse HEAD
    git -C "${SPDLOG_SRC}" status --short
  } > "${EVIDENCE}/toolchain-and-deps.txt" 2>&1
}

write_manifest_and_leak_check() {
  (
    cd "${SNAPSHOT}" &&
    find . -type f -print0 | sort -z | xargs -0 sha256sum
  ) > "${EVIDENCE}/guest-source-manifest.sha256.txt"
  sha256sum "${EVIDENCE}/guest-source-manifest.sha256.txt" > "${EVIDENCE}/guest-source-manifest-file.sha256.txt"
  {
    echo "snapshot=${SNAPSHOT}"
    echo "file_count=$(find "${SNAPSHOT}" -type f | wc -l)"
    echo "size_bytes=$(du -sb "${SNAPSHOT}" | awk '{print $1}')"
    echo
    echo "hidden_dirs:"
    find "${SNAPSHOT}" -type d -name '.*' -print
    echo
    echo "build_dirs:"
    find "${SNAPSHOT}" -type d \( -name 'build*' -o -name '_deps' -o -name 'CMakeFiles' \) -print
    echo
    echo "forbidden_files:"
    find "${SNAPSHOT}" -type f \( \
      -name '*.exe' -o -name '*.dll' -o -name '*.pdb' -o -name '*.ilk' -o -name '*.obj' -o \
      -name '*.o' -o -name '*.lib' -o -name '*.a' -o -name '*.so' -o -name '*.dylib' -o \
      -name '*.zip' -o -name '*.7z' -o -name '*.tar' -o -name '*.gz' -o -name '*.xz' -o \
      -name '*.bz2' -o -name '*.bin' -o -name '*.tmp' -o -name '*.log' -o \
      -name 'CMakeCache.txt' -o -name 'compile_commands.json' \) -print
  } > "${EVIDENCE}/snapshot-leak-check.txt"
}

configure_build_ctest() {
  local name="$1"
  local preset="$2"
  local build_type="$3"
  local sanitizer="$4"
  local build_dir="${BUILD_ROOT}/${name}"
  local ctest_regex="${5:-}"
  local cxx26="${6:-OFF}"
  local cxx29="${7:-OFF}"
  mkdir -p "${build_dir}"

  local configure_cmd=(
    cmake -S "${SRC}" -B "${build_dir}" --preset "${preset}"
    -DCMAKE_BUILD_TYPE="${build_type}"
    -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON
    -DCONCURRENCY_STUDY_TEST_FAST_MATH=OFF
    -DCONCURRENCY_STUDY_FETCH_DEPS=OFF
    -DCONCURRENCY_STUDY_ENABLE_SPDLOG=ON
    -DCONCURRENCY_STUDY_FMT_SOURCE_DIR="${FMT_SRC}"
    -DCONCURRENCY_STUDY_SPDLOG_SOURCE_DIR="${SPDLOG_SRC}"
    -DCONCURRENCY_STUDY_ENABLE_XSIMD=OFF
    -DCONCURRENCY_STUDY_ENABLE_STDEXEC=OFF
    -DCONCURRENCY_STUDY_ENABLE_CXX26="${cxx26}"
    -DCONCURRENCY_STUDY_ENABLE_CXX29="${cxx29}"
    -DCONCURRENCY_STUDY_BUILD_BENCHMARKS=OFF
    -DCONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS=OFF
    -DCONCURRENCY_STUDY_SANITIZER="${sanitizer}"
  )
  if [[ "${name}" == frontier-* ]]; then
    configure_cmd+=(-DCMAKE_CXX_COMPILER=clang++-18)
  fi

  record_command_allow_fail "${name}-configure" 300 "${configure_cmd[@]}"
  record_command_allow_fail "${name}-build" 1200 cmake --build "${build_dir}" --parallel 4
  record_command_allow_fail "${name}-ctest-list" 120 ctest --test-dir "${build_dir}" -C "${build_type}" --show-only=json-v1
  mv "${EVIDENCE}/${name}-ctest-list.txt" "${EVIDENCE}/${name}-ctest-list-output.txt"
  ctest --test-dir "${build_dir}" -C "${build_type}" --show-only=json-v1 > "${EVIDENCE}/${name}-ctest-list.json" 2> "${EVIDENCE}/${name}-ctest-list-stderr.txt" || true

  local ctest_cmd=(ctest --test-dir "${build_dir}" -C "${build_type}" --output-on-failure --output-junit "${EVIDENCE}/${name}-junit.xml" --timeout 120)
  if [[ -n "${ctest_regex}" ]]; then
    ctest_cmd+=(-R "${ctest_regex}")
  fi
  record_command_allow_fail "${name}-ctest" 1200 "${ctest_cmd[@]}"
}

summarize_results() {
  python3 - <<'PY' > "${EVIDENCE}/result-summary.json"
import json
import pathlib
import xml.etree.ElementTree as ET

evidence = pathlib.Path("/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/final-20260910-173010-d5356576")
commands = []
with (evidence / "commands.jsonl").open(encoding="utf-8") as f:
    for line in f:
        commands.append(json.loads(line))

configs = {}
for xml_path in sorted(evidence.glob("*-junit.xml")):
    name = xml_path.name.removesuffix("-junit.xml")
    try:
        root = ET.parse(xml_path).getroot()
    except ET.ParseError as exc:
        configs[name] = {"junit": str(xml_path.name), "parseError": str(exc)}
        continue
    testcases = root.findall(".//testcase")
    skipped = [tc.get("name", "") for tc in testcases if tc.find("skipped") is not None]
    failures = [tc.get("name", "") for tc in testcases if tc.find("failure") is not None or tc.find("error") is not None]
    configs[name] = {
        "junit": xml_path.name,
        "tests": len(testcases),
        "skipped": len(skipped),
        "failed": len(failures),
        "skippedTests": skipped,
        "failedTests": failures,
    }

registration = {}
for path in sorted(evidence.glob("*-ctest-list.json")):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        names = [t.get("name", "") for t in data.get("tests", [])]
    except Exception as exc:
        registration[path.name] = {"error": str(exc)}
        continue
    registration[path.name] = {
        "testCount": len(names),
        "has_runtime_benchmark_tools": "runtime_benchmark_tools" in names,
        "has_runtime_materials": "runtime_materials" in names,
        "nativeFrontierTests": [n for n in names if n.startswith("F01_") or n.startswith("F02_") or n.startswith("F03_")],
    }

out = {
    "commands": commands,
    "configs": configs,
    "registration": registration,
}
print(json.dumps(out, indent=2, ensure_ascii=False))
PY
}

write_toolchain
write_manifest_and_leak_check

configure_build_ctest "gcc-release" "linux-core" "Release" "none"
configure_build_ctest "gcc-debug" "linux-debug" "Debug" "none"
configure_build_ctest "clang-asan-ubsan" "linux-asan" "RelWithDebInfo" "address"
configure_build_ctest "clang-tsan" "linux-tsan" "RelWithDebInfo" "thread"
configure_build_ctest "frontier-clang18" "native-cxx29" "Release" "none" \
  '^(F01_thread_attributes_|F02_hazard_pointer_batches_|F03_std_(senders|hazard_pointer|rcu)_reference)' "ON" "ON"

summarize_results
