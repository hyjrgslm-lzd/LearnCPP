#!/usr/bin/env bash
set -u

work_dir="/root/learncpp-c08-builds/m1-tsan-diagnosis-20260910-1815"
report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815"

mkdir -p "$work_dir" "$report"
cd "$work_dir" || exit 2
pwd > "$report/queue-pwd.txt"

cat > single_worker_queue_exception_probe.cpp <<'CPP'
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

class single_worker_queue {
    using task = std::function<void()>;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<task> tasks_;
    bool closed_ = false;
    std::thread worker_;

    void run() {
        for (;;) {
            task work;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [&] { return closed_ || !tasks_.empty(); });
                if (tasks_.empty()) return;
                work = std::move(tasks_.front());
                tasks_.pop_front();
            }
            work();
        }
    }

public:
    single_worker_queue() : worker_([this] { run(); }) {}
    ~single_worker_queue() { join(); }

    template<class F>
    auto submit(F&& f) {
        using result = std::invoke_result_t<std::decay_t<F>&>;
        auto packaged = std::make_shared<std::packaged_task<result()>>(std::forward<F>(f));
        auto future = packaged->get_future();
        task wrapped = [packaged] { (*packaged)(); };
        {
            std::lock_guard lock(mutex_);
            tasks_.push_back(std::move(wrapped));
        }
        cv_.notify_one();
        return future;
    }

    void join() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        cv_.notify_one();
        if (worker_.joinable()) worker_.join();
    }
};

int exception_round(bool join_before_what) {
    single_worker_queue queue;
    auto error = queue.submit([]() -> int { throw std::runtime_error("task-error"); });
    bool caught = false;
    if (join_before_what) {
        error.wait();
        queue.join();
    }
    try {
        (void)error.get();
    } catch (const std::runtime_error& e) {
        caught = std::string_view(e.what()) == "task-error";
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        caught = caught && std::string_view(e.what()) == "task-error";
    }
    queue.join();
    return caught ? 0 : 1;
}

int value_round() {
    single_worker_queue queue;
    auto answer = queue.submit([] { return 42; });
    const int value = answer.get();
    queue.join();
    return value == 42 ? 0 : 1;
}

int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "exception";
    const int rounds = argc > 2 ? std::stoi(argv[2]) : 200;
    for (int i = 0; i < rounds; ++i) {
        int rc = 1;
        if (mode == "exception") rc = exception_round(false);
        else if (mode == "join-before-what") rc = exception_round(true);
        else if (mode == "value") rc = value_round();
        else return 2;
        if (rc != 0) return rc;
    }
    std::cout << "ok " << mode << " rounds=" << rounds << '\n';
}
CPP

clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer single_worker_queue_exception_probe.cpp -pthread -o single_worker_queue_tsan > "$report/build-queue-probe-tsan.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-queue-probe-tsan.txt"

clang++-18 -std=c++23 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer single_worker_queue_exception_probe.cpp -pthread -o single_worker_queue_asan > "$report/build-queue-probe-asan.txt" 2>&1
printf '%s\n' "$?" > "$report/status-build-queue-probe-asan.txt"

run_case() {
    label="$1"
    shift
    /usr/bin/timeout 30 "$@" > "$report/run-${label}.txt" 2>&1
    printf '%s\n' "$?" > "$report/status-${label}.txt"
}

if [ -x ./single_worker_queue_tsan ]; then
    for mode in exception join-before-what value; do
        run_case "queue-tsan-${mode}" ./single_worker_queue_tsan "$mode" 300
    done
    for n in 01 02 03 04 05 06 07 08 09 10; do
        run_case "queue-tsan-exception-repeat-${n}" ./single_worker_queue_tsan exception 300
    done
fi

if [ -x ./single_worker_queue_asan ]; then
    for mode in exception join-before-what value; do
        run_case "queue-asan-${mode}" ./single_worker_queue_asan "$mode" 300
    done
fi

cp single_worker_queue_exception_probe.cpp "$report"/

rm -f "$report/queue-status-summary.txt" "$report/queue-status-summary.json"
for f in "$report"/status-queue-*.txt "$report"/status-build-queue-*.txt; do
    [ -e "$f" ] || continue
    printf '%s=' "$(basename "$f")"
    cat "$f"
done | sort > "$report/queue-status-summary.txt"

python3 - <<PY
import json
from pathlib import Path
report = Path("$report")
statuses = {p.name: p.read_text(encoding="utf-8", errors="replace").strip()
            for p in sorted(list(report.glob("status-queue-*.txt")) + list(report.glob("status-build-queue-*.txt")))}
(report / "queue-status-summary.json").write_text(json.dumps(statuses, indent=2, sort_keys=True) + "\\n", encoding="utf-8")
PY

cat "$report/queue-status-summary.txt"
