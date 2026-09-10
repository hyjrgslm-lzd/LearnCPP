#!/usr/bin/env bash
set -euo pipefail
# Run inside the guest. Passing a script avoids Windows/bash nested quoting.
run_id=${1:?supply a new run id}
profile=${2:?supply foundation, core, debug, asan, asan-uring, uring, uring-debug or student}
[[ "$run_id" =~ ^[a-zA-Z0-9_-]+$ ]] || { echo 'invalid run id' >&2; exit 1; }
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
task=/root/learncpp-c07
source_repo=/mnt/f/CPPTrain/LearnCPP
snapshot="$task/snapshots/$run_id"
build="$task/builds/$run_id/$profile"
evidence="$task/evidence/$run_id"
published="$source_repo/C07_OS_Memory_System_IO/references/validation/$run_id"
[ ! -e "$evidence" ] && [ ! -e "$published" ] || { echo 'run evidence already exists' >&2; exit 1; }
config=Release
options=(-DC07_STUDY_ENABLE_IO_URING=OFF -DC07_STUDY_BUILD_REFERENCE=ON)
case "$profile" in
    foundation|core) ;;
    debug) config=Debug ;;
    asan) config=RelWithDebInfo; options+=(-DC07_STUDY_ENABLE_ASAN=ON) ;;
    asan-uring) config=RelWithDebInfo; options+=(-DC07_STUDY_ENABLE_ASAN=ON -DC07_STUDY_ENABLE_IO_URING=ON
        -DC07_LIBURING_ROOT="$task/deps/install-liburing-2.15") ;;
    uring|uring-debug)
        options+=(-DC07_STUDY_ENABLE_IO_URING=ON -DC07_LIBURING_ROOT="$task/deps/install-liburing-2.15")
        if [ "$profile" = uring-debug ]; then config=Debug; fi ;;
    student) options+=(-DC07_STUDY_BUILD_REFERENCE=OFF -DC07_STUDY_TEST_STUDENTS=ON -DC07_STUDY_TRACE_INCLUDES=ON) ;;
    *) echo 'unsupported profile' >&2; exit 1 ;;
esac
mkdir -p "$evidence"
publish() {
    status=$?
    mkdir -p "$published"
    cp -R "$evidence/." "$published/"
    if [ -d "$build/records" ]; then cp -R "$build/records" "$published/records"; fi
    exit "$status"
}
trap publish EXIT
python3 "$source_repo/C07_OS_Memory_System_IO/exercises/tools/snapshot_linux.py" --run-id "$run_id"
cp "$snapshot/snapshot-manifest.json" "$evidence/snapshot-manifest.json"
record="$snapshot/C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py"
course="$snapshot/C07_OS_Memory_System_IO/exercises"
mkdir -p "$build/.cmake/api/v1/query"
: > "$build/.cmake/api/v1/query/codemodel-v2"
cd "$course"
python3 "$record" --output "$evidence/configure.json" --timeout 180 -- \
    cmake -S "$course" -B "$build" -DCMAKE_BUILD_TYPE="$config" "${options[@]}"
targets=()
if [ "$profile" = foundation ]; then
    targets=(--target L01_handles_reference L01_handles_validation_good L01_handles_validation_bad
        L01_handles_ownership L01_thread_process
        L02_sync_io_reference L02_sync_io_validation_good L02_sync_io_validation_bad)
elif [ "$profile" = student ]; then
    mapfile -t students < "$build/student-targets-$config.txt"
    targets=(--clean-first --target "${students[@]}")
fi
python3 "$record" --output "$evidence/build.json" --timeout 900 -- \
    cmake --build "$build" --config "$config" --parallel 4 "${targets[@]}"
if [ "$profile" = student ]; then
    python3 "$course/tools/verify_students.py" --build "$build" --config "$config" \
        --trace "$evidence/build.json" --output "$evidence/student-verification.json"
else
    tests=()
    if [ "$profile" = foundation ]; then tests=(-R '^L0[12]_'); fi
    python3 "$record" --output "$evidence/ctest.json" --timeout 600 -- \
        ctest --test-dir "$build" -C "$config" --output-on-failure "${tests[@]}" --output-junit "$evidence/ctest.xml"
fi
python3 "$course/tools/record_environment.py" --build "$build" --output "$evidence/environment.json"
