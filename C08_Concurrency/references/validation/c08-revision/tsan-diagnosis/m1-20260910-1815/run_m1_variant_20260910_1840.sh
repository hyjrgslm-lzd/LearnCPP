#!/usr/bin/env bash
set -u

src="/root/learncpp-c08-builds/m1-variant-src-20260910-1840"
build="/root/learncpp-c08-builds/m1-variant-tsan-20260910-1840"
report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815"

mkdir -p "$report"
rm -rf "$src" "$build"
mkdir -p "$src"
(
    cd /root/learncpp-c08-src/final-20260910-173010-d5356576 || exit 2
    tar --exclude='C08_Concurrency/exercises/build*' -cf - C08_Concurrency/exercises
) | (
    cd "$src" || exit 2
    tar -xf -
)

cd "$src/C08_Concurrency/exercises/M1_work_stealing_pool" || exit 2
cp solution.cpp solution.original.cpp
python3 - <<'PY'
from pathlib import Path
p = Path("solution.cpp")
s = p.read_text()
old = '        try { error.get(); } catch (const std::runtime_error& e) { caught = std::string_view(e.what()) == "task-error"; }'
new = '''        error.wait();
        pool.join();
        try { error.get(); } catch (const std::runtime_error& e) { caught = std::string_view(e.what()) == "task-error"; }'''
if old not in s:
    raise SystemExit("pattern not found")
p.write_text(s.replace(old, new))
PY
diff -u solution.original.cpp solution.cpp > "$report/m1-variant-join-before-what.diff" || true

cd "$src" || exit 2
sha256sum \
    C08_Concurrency/exercises/M1_work_stealing_pool/solution.original.cpp \
    C08_Concurrency/exercises/M1_work_stealing_pool/solution.cpp \
    C08_Concurrency/exercises/include/concurrency_study/work_stealing_pool.hpp \
    > "$report/m1-variant-source-hashes.txt"

cmake -S "$src/C08_Concurrency/exercises" -B "$build" -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON \
    -DCONCURRENCY_STUDY_TEST_STARTERS=OFF \
    -DCONCURRENCY_STUDY_SANITIZER=thread \
    > "$report/m1-variant-configure.txt" 2>&1
printf '%s\n' "$?" > "$report/status-m1-variant-configure.txt"

cmake --build "$build" --target M1_work_stealing_pool_reference \
    > "$report/m1-variant-build.txt" 2>&1
printf '%s\n' "$?" > "$report/status-m1-variant-build.txt"

if [ -x "$build/M1_work_stealing_pool/M1_work_stealing_pool_reference" ]; then
    for n in 01 02 03 04 05 06 07 08 09 10; do
        /usr/bin/timeout 30 "$build/M1_work_stealing_pool/M1_work_stealing_pool_reference" > "$report/m1-variant-run-$n.txt" 2>&1
        printf '%s\n' "$?" > "$report/status-m1-variant-run-$n.txt"
    done
fi

for f in "$report"/status-m1-variant-*.txt; do
    [ -e "$f" ] || continue
    printf '%s=' "$(basename "$f")"
    cat "$f"
done | sort > "$report/m1-variant-status-summary.txt"

cat "$report/m1-variant-status-summary.txt"
