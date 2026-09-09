#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"

#include <chrono>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <stop_token>
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
    std::stop_token st;
    bool await_ready() const noexcept { return d <= 0ms || st.stop_requested(); }
    void await_suspend(std::coroutine_handle<> h) const {
        workers.submit([h, d = d, st = st] {
            auto end = std::chrono::steady_clock::now() + d;
            while (!st.stop_requested() && std::chrono::steady_clock::now() < end) {
                std::this_thread::sleep_for(2ms);
            }
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

struct task_cancelled : std::exception {
    const char* what() const noexcept override { return "task cancelled"; }
};

coroutine_study::lazy_task<int> batch_process(worker_group& workers, std::stop_token st, int total) {
    int processed = 0;
    for (int i = 0; i < total; ++i) {
        if (st.stop_requested()) co_return processed;
        co_await timer_awaiter{workers, 20ms, st};
        if (st.stop_requested()) co_return processed;
        ++processed;
    }
    co_return processed;
}

coroutine_study::lazy_task<int> batch_process_throwing(worker_group& workers, std::stop_token st, int total) {
    int processed = 0;
    for (int i = 0; i < total; ++i) {
        if (st.stop_requested()) throw task_cancelled{};
        co_await timer_awaiter{workers, 20ms, st};
        if (st.stop_requested()) throw task_cancelled{};
        ++processed;
    }
    co_return processed;
}

} // namespace

int main() {
    using coroutine_study::check;

    worker_group workers;
    std::stop_source src;
    auto t = batch_process(workers, src.get_token(), 10);
    std::jthread canceller([&] {
        std::this_thread::sleep_for(55ms);
        src.request_stop();
    });
    int done = coroutine_study::sync_wait(std::move(t));
    workers.join();
    check(done > 0 && done < 10, "stop_token stops between batches");

    std::stop_source throwing_src;
    auto throwing = batch_process_throwing(workers, throwing_src.get_token(), 10);
    std::jthread throwing_canceller([&] {
        std::this_thread::sleep_for(30ms);
        throwing_src.request_stop();
    });
    bool thrown = false;
    try {
        (void)coroutine_study::sync_wait(std::move(throwing));
    } catch (const task_cancelled&) {
        thrown = true;
    }
    workers.join();
    check(thrown, "throwing cancellation maps to exception path");
    std::cout << "C1_reference OK\n";
}
