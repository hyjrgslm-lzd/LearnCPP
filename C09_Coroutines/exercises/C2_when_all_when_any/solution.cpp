#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"
#include "coroutine_study/runtime.hpp"

#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <tuple>
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

coroutine_study::lazy_task<int> delayed(
    worker_group& workers,
    int value,
    std::chrono::milliseconds d,
    std::stop_token st = {}
) {
    co_await timer_awaiter{workers, d, st};
    if (st.stop_requested()) co_return -1;
    if (value == 500) throw std::runtime_error("fetch failed");
    co_return value;
}

coroutine_study::lazy_task<std::tuple<int, int, int>> when_all3(
    coroutine_study::lazy_task<int> a,
    coroutine_study::lazy_task<int> b,
    coroutine_study::lazy_task<int> c,
    std::stop_source& stop_source
) {
    std::optional<int> ra, rb, rc;
    std::exception_ptr error;
    std::mutex error_mutex;

    auto run = [&](auto& task, auto& out) {
        try {
            out = coroutine_study::sync_wait(std::move(task));
        } catch (...) {
            std::lock_guard lock(error_mutex);
            if (!error) error = std::current_exception();
            stop_source.request_stop();
        }
    };

    std::jthread ta([&] { run(a, ra); });
    std::jthread tb([&] { run(b, rb); });
    std::jthread tc([&] { run(c, rc); });
    ta.join();
    tb.join();
    tc.join();
    if (error) std::rethrow_exception(error);
    co_return std::make_tuple(*ra, *rb, *rc);
}

} // namespace

int main() {
    using coroutine_study::check;

    worker_group workers;
    std::stop_source all_src;
    auto start = std::chrono::steady_clock::now();
    auto [a, b, c] = coroutine_study::sync_wait(
        when_all3(delayed(workers, 100, 50ms, all_src.get_token()),
                  delayed(workers, 200, 150ms, all_src.get_token()),
                  delayed(workers, 300, 300ms, all_src.get_token()),
                  all_src));
    workers.join();
    auto dt = std::chrono::steady_clock::now() - start;
    check(a == 100 && b == 200 && c == 300, "when_all values");
    check(dt < 430ms, "when_all runs concurrently");

    std::stop_source any_src;
    auto any = coroutine_study::when_any_cancel_join(
        any_src,
        delayed(workers, 300, 300ms, any_src.get_token()),
        delayed(workers, -1, 80ms, any_src.get_token())
    );
    workers.join();
    check(any.index == 1 && any.value == -1, "when_any timeout wins");

    std::stop_source fail_src;
    bool thrown = false;
    try {
        (void)coroutine_study::sync_wait(
            when_all3(delayed(workers, 100, 200ms, fail_src.get_token()),
                      delayed(workers, 500, 20ms, fail_src.get_token()),
                      delayed(workers, 300, 200ms, fail_src.get_token()),
                      fail_src));
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    workers.join();
    check(thrown && fail_src.stop_requested(), "when_all requests cancellation and rethrows child error");
    std::cout << "C2_reference OK\n";
}
