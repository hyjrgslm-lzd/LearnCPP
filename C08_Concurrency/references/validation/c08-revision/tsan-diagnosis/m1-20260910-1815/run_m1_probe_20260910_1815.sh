#!/usr/bin/env bash
set -u

work_dir="/root/learncpp-c08-builds/m1-tsan-diagnosis-20260910-1815"
report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815"
src="/root/learncpp-c08-src/final-20260910-173010-d5356576"
build_tsan="/root/learncpp-c08-builds/final-20260910-173010-d5356576/clang-tsan"

mkdir -p "$work_dir" "$report"
cd "$work_dir" || exit 2
pwd > "$report/pwd.txt"

cat > packaged_task_exception_probe.cpp <<'CPP'
#include <cassert>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

int run_exception_once() {
    auto packaged = std::make_shared<std::packaged_task<int()>>(
        []() -> int { throw std::runtime_error("task-error"); });
    auto future = packaged->get_future();
    std::function<void()> work = [packaged] { (*packaged)(); };
    std::thread worker([work = std::move(work)] mutable {
        work();
    });
    bool caught = false;
    try {
        (void)future.get();
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "task-error";
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        caught = caught && std::string_view(e.what()) == "task-error";
    }
    worker.join();
    return caught ? 0 : 1;
}

int run_value_once() {
    auto packaged = std::make_shared<std::packaged_task<int()>>([] { return 42; });
    auto future = packaged->get_future();
    std::function<void()> work = [packaged] { (*packaged)(); };
    std::thread worker([work = std::move(work)] mutable {
        work();
    });
    const int value = future.get();
    worker.join();
    return value == 42 ? 0 : 1;
}

int run_exception_join_before_get_once() {
    auto packaged = std::make_shared<std::packaged_task<int()>>(
        []() -> int { throw std::runtime_error("task-error"); });
    auto future = packaged->get_future();
    std::function<void()> work = [packaged] { (*packaged)(); };
    std::thread worker([work = std::move(work)] mutable {
        work();
    });
    worker.join();
    try {
        (void)future.get();
    } catch (const std::runtime_error& e) {
        return std::string_view(e.what()) == "task-error" ? 0 : 1;
    }
    return 1;
}

int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "exception";
    const int rounds = argc > 2 ? std::stoi(argv[2]) : 200;
    for (int i = 0; i < rounds; ++i) {
        int rc = 1;
        if (mode == "exception") rc = run_exception_once();
        else if (mode == "value") rc = run_value_once();
        else if (mode == "join-before-get") rc = run_exception_join_before_get_once();
        else return 2;
        if (rc != 0) return rc;
    }
    std::cout << "ok " << mode << " rounds=" << rounds << '\n';
}
CPP

{
    printf 'work_dir=%s\n' "$work_dir"
    printf 'src=%s\n' "$src"
    printf 'build_tsan=%s\n' "$build_tsan"
    clang++-18 --version | head -n 1
    g++ --version | head -n 1
    ldd --version | head -n 1
} > "$report/environment.txt"

sha256sum \
    "$src/C08_Concurrency/exercises/M1_work_stealing_pool/solution.cpp" \
    "$src/C08_Concurrency/exercises/include/concurrency_study/work_stealing_pool.hpp" \
    > "$report/source-hashes.txt"

clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer packaged_task_exception_probe.cpp -pthread -o packaged_task_exception_tsan > "$report/build-packaged-task-tsan.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-packaged-task-tsan.txt"

clang++-18 -std=c++23 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer packaged_task_exception_probe.cpp -pthread -o packaged_task_exception_asan > "$report/build-packaged-task-asan.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-packaged-task-asan.txt"

clang++-18 -std=c++23 -O1 -g packaged_task_exception_probe.cpp -pthread -o packaged_task_exception_plain > "$report/build-packaged-task-plain.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-packaged-task-plain.txt"

run_case() {
    label="$1"
    shift
    /usr/bin/timeout 30 "$@" > "$report/run-${label}.txt" 2>&1
    printf '%s\n' "$?" > "$report/status-${label}.txt"
}

if [ -x ./packaged_task_exception_tsan ]; then
    for mode in exception value join-before-get; do
        run_case "tsan-${mode}" ./packaged_task_exception_tsan "$mode" 200
    done
    for n in 01 02 03 04 05 06 07 08 09 10; do
        run_case "tsan-exception-repeat-${n}" ./packaged_task_exception_tsan exception 200
    done
fi

if [ -x ./packaged_task_exception_asan ]; then
    for mode in exception value join-before-get; do
        run_case "asan-${mode}" ./packaged_task_exception_asan "$mode" 200
    done
fi

if [ -x ./packaged_task_exception_plain ]; then
    for mode in exception value join-before-get; do
        run_case "plain-${mode}" ./packaged_task_exception_plain "$mode" 200
    done
fi

if [ -x "$build_tsan/M1_work_stealing_pool/M1_work_stealing_pool_reference" ]; then
    for n in 01 02 03 04 05; do
        run_case "existing-M1-tsan-${n}" "$build_tsan/M1_work_stealing_pool/M1_work_stealing_pool_reference"
    done
fi

cp packaged_task_exception_probe.cpp "$report"/

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
            if p.name != "status-summary.txt"}
(report / "status-summary.json").write_text(json.dumps(statuses, indent=2, sort_keys=True) + "\\n", encoding="utf-8")
PY

cat "$report/status-summary.txt"
