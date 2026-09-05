#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"
#include "coroutine_study/runtime.hpp"

#include <atomic>
#include <chrono>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

struct worker_group {
    ~worker_group() { join(); }
    template <class Fn>
    void submit(Fn&& fn) {
        std::lock_guard lock(mutex);
        workers.emplace_back(std::forward<Fn>(fn));
    }
    void join() {
        for (;;) {
            std::vector<std::jthread> local;
            {
                std::lock_guard lock(mutex);
                if (workers.empty()) break;
                local.swap(workers);
            }
            for (auto& worker : local) if (worker.joinable()) worker.join();
        }
    }
    std::mutex mutex;
    std::vector<std::jthread> workers;
};

struct timer_awaiter {
    worker_group& workers;
    std::chrono::milliseconds d;
    bool await_ready() const noexcept { return d <= 0ms; }
    void await_suspend(std::coroutine_handle<> h) const {
        workers.submit([h, d = d] {
            std::this_thread::sleep_for(d);
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

coroutine_study::lazy_task<void> worker(worker_group& workers, std::atomic<int>& completed, int delay_ms) {
    co_await timer_awaiter{workers, std::chrono::milliseconds(delay_ms)};
    ++completed;
}

coroutine_study::lazy_task<void> failing(worker_group& workers) {
    co_await timer_awaiter{workers, 10ms};
    throw std::runtime_error("scope failure");
}

} // namespace

int main() {
    using coroutine_study::check;

    worker_group workers;
    std::atomic<int> completed = 0;
    {
        coroutine_study::task_scope scope;
        for (int i = 0; i < 5; ++i) scope.spawn(worker(workers, completed, i * 10));
        scope.join();
    }
    workers.join();
    check(completed == 5, "scope joins spawned tasks");

    bool thrown = false;
    try {
        coroutine_study::task_scope scope;
        scope.spawn(failing(workers));
        scope.join();
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    workers.join();
    check(thrown, "scope propagates child exception");
    std::cout << "C3_reference OK\n";
}
