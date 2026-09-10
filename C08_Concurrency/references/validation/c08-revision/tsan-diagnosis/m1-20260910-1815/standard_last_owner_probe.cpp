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
