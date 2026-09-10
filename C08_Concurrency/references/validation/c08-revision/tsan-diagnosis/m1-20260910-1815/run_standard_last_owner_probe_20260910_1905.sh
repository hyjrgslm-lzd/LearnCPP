#!/usr/bin/env bash
set -u

work_dir="/root/learncpp-c08-builds/m1-standard-last-owner-20260910-1905"
report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815"

mkdir -p "$work_dir" "$report"
cd "$work_dir" || exit 2
pwd > "$report/standard-last-owner-pwd.txt"

cat > standard_last_owner_probe.cpp <<'CPP'
#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

struct outcome {
    int rc = 1;
    int worker_started = 0;
    int worker_finished_task = 0;
    int worker_destroyed_task = 0;
};

int exception_worker_last_owner_once() {
    std::atomic<bool> main_finished_what{false};
    std::atomic<int> worker_started{0};
    std::atomic<int> worker_finished_task{0};
    std::atomic<int> worker_destroyed_task{0};

    std::packaged_task<int()> task([]() -> int { throw std::runtime_error("task-error"); });
    auto future = task.get_future();

    std::thread worker([task = std::move(task), &main_finished_what, &worker_started,
                        &worker_finished_task, &worker_destroyed_task]() mutable {
        worker_started.store(1, std::memory_order_relaxed);
        task();
        worker_finished_task.store(1, std::memory_order_relaxed);
        while (!main_finished_what.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
        }
        worker_destroyed_task.store(1, std::memory_order_relaxed);
    });

    bool caught = false;
    try {
        (void)future.get();
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "task-error";
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        caught = caught && std::string_view(e.what()) == "task-error";
    } catch (...) {
        main_finished_what.store(true, std::memory_order_relaxed);
        worker.join();
        throw;
    }

    main_finished_what.store(true, std::memory_order_relaxed);
    worker.join();
    return caught &&
        worker_started.load(std::memory_order_relaxed) == 1 &&
        worker_finished_task.load(std::memory_order_relaxed) == 1 &&
        worker_destroyed_task.load(std::memory_order_relaxed) == 1 ? 0 : 1;
}

int value_worker_last_owner_once() {
    std::atomic<bool> main_got_value{false};
    std::packaged_task<int()> task([] { return 42; });
    auto future = task.get_future();

    std::thread worker([task = std::move(task), &main_got_value]() mutable {
        task();
        while (!main_got_value.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
        }
    });

    const int value = future.get();
    main_got_value.store(true, std::memory_order_relaxed);
    worker.join();
    return value == 42 ? 0 : 1;
}

int exception_join_before_get_once() {
    std::packaged_task<int()> task([]() -> int { throw std::runtime_error("task-error"); });
    auto future = task.get_future();
    std::thread worker([task = std::move(task)]() mutable { task(); });
    worker.join();
    try {
        (void)future.get();
    } catch (const std::runtime_error& e) {
        return std::string_view(e.what()) == "task-error" ? 0 : 1;
    }
    return 1;
}

int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "exception-worker-last-owner";
    const int rounds = argc > 2 ? std::stoi(argv[2]) : 200;
    for (int i = 0; i < rounds; ++i) {
        int rc = 1;
        if (mode == "exception-worker-last-owner") rc = exception_worker_last_owner_once();
        else if (mode == "value-worker-last-owner") rc = value_worker_last_owner_once();
        else if (mode == "exception-join-before-get") rc = exception_join_before_get_once();
        else return 2;
        if (rc != 0) return rc;
    }
    std::cout << "ok " << mode << " rounds=" << rounds
              << " owner=worker-last-task-ref cleanup=worker-thread gate=relaxed-atomic\n";
}
CPP

{
    printf 'work_dir=%s\n' "$work_dir"
    clang++-18 --version | head -n 1
    g++ --version | head -n 1
    ldd --version | head -n 1
} > "$report/standard-last-owner-environment.txt"

clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer standard_last_owner_probe.cpp -pthread -o standard_last_owner_tsan > "$report/build-standard-last-owner-tsan.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-standard-last-owner-tsan.txt"

clang++-18 -std=c++23 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer standard_last_owner_probe.cpp -pthread -o standard_last_owner_asan > "$report/build-standard-last-owner-asan.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-standard-last-owner-asan.txt"

clang++-18 -std=c++23 -O1 -g standard_last_owner_probe.cpp -pthread -o standard_last_owner_plain > "$report/build-standard-last-owner-plain.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-standard-last-owner-plain.txt"

run_case() {
    label="$1"
    shift
    /usr/bin/timeout 30 "$@" > "$report/run-${label}.txt" 2>&1
    printf '%s\n' "$?" > "$report/status-${label}.txt"
}

if [ -x ./standard_last_owner_tsan ]; then
    for mode in exception-worker-last-owner value-worker-last-owner exception-join-before-get; do
        run_case "standard-last-owner-tsan-${mode}" ./standard_last_owner_tsan "$mode" 300
    done
    for n in 01 02 03 04 05 06 07 08 09 10; do
        run_case "standard-last-owner-tsan-exception-repeat-${n}" ./standard_last_owner_tsan exception-worker-last-owner 300
    done
fi

if [ -x ./standard_last_owner_asan ]; then
    for mode in exception-worker-last-owner value-worker-last-owner exception-join-before-get; do
        run_case "standard-last-owner-asan-${mode}" ./standard_last_owner_asan "$mode" 300
    done
fi

if [ -x ./standard_last_owner_plain ]; then
    for mode in exception-worker-last-owner value-worker-last-owner exception-join-before-get; do
        run_case "standard-last-owner-plain-${mode}" ./standard_last_owner_plain "$mode" 300
    done
fi

cp standard_last_owner_probe.cpp "$report"/

rm -f "$report/standard-last-owner-status-summary.txt" "$report/standard-last-owner-status-summary.json"
for f in "$report"/status-standard-last-owner-*.txt "$report"/status-build-standard-last-owner-*.txt; do
    [ -e "$f" ] || continue
    printf '%s=' "$(basename "$f")"
    cat "$f"
done | sort > "$report/standard-last-owner-status-summary.txt"

python3 - <<PY
import json
from pathlib import Path
report = Path("$report")
statuses = {p.name: p.read_text(encoding="utf-8", errors="replace").strip()
            for p in sorted(list(report.glob("status-standard-last-owner-*.txt")) + list(report.glob("status-build-standard-last-owner-*.txt")))}
(report / "standard-last-owner-status-summary.json").write_text(json.dumps(statuses, indent=2, sort_keys=True) + "\\n", encoding="utf-8")
PY

cat "$report/standard-last-owner-status-summary.txt"
