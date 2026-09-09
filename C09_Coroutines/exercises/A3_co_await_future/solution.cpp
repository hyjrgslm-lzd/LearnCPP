#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"

#include <chrono>
#include <coroutine>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
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
            for (auto& worker : local) {
                if (worker.joinable()) worker.join();
            }
        }
    }
    std::mutex mutex;
    std::vector<std::jthread> workers;
};

template <class T>
struct future_state {
    std::mutex mutex;
    std::optional<T> value;
    std::exception_ptr error;
    // 每份观察状态用于一次等待；信号让实验精确控制 producer 的完成时机。
    std::promise<void> waiting_started;
    bool ready_path = false;
    bool suspended = false;
    std::thread::id caller_thread;
    std::thread::id worker_thread;
    std::thread::id resume_thread;
};

template <class T>
struct future_awaiter {
    std::future<T> fut;
    worker_group& workers;
    future_state<T>& state;

    bool await_ready() {
        bool ready = fut.wait_for(0s) == std::future_status::ready;
        state.ready_path = ready;
        return ready;
    }
    void await_suspend(std::coroutine_handle<> h) {
        state.suspended = true;
        state.caller_thread = std::this_thread::get_id();
        state.waiting_started.set_value();
        auto* state_ptr = &state;
        workers.submit([h, fut = std::move(fut), state_ptr]() mutable {
            state_ptr->worker_thread = std::this_thread::get_id();
            try {
                T value = fut.get();
                {
                    std::lock_guard lock(state_ptr->mutex);
                    state_ptr->value.emplace(std::move(value));
                }
            } catch (...) {
                std::lock_guard lock(state_ptr->mutex);
                state_ptr->error = std::current_exception();
            }
            state_ptr->resume_thread = std::this_thread::get_id();
            h.resume();
        });
    }
    T await_resume() {
        if (state.ready_path) return fut.get();
        std::lock_guard lock(state.mutex);
        if (state.error) std::rethrow_exception(state.error);
        return std::move(*state.value);
    }
};

template <class T>
future_awaiter<T> await_future(std::future<T> fut, worker_group& workers, future_state<T>& state) {
    return future_awaiter<T>{std::move(fut), workers, state};
}

coroutine_study::lazy_task<int> ok_path(worker_group& workers, future_state<int>& state) {
    auto fut = std::async(std::launch::async, [started = state.waiting_started.get_future()]() mutable {
        started.wait();
        return 42;
    });
    int result = co_await await_future(std::move(fut), workers, state);
    co_return result * 2;
}

coroutine_study::lazy_task<int> ready_path(worker_group& workers, future_state<int>& state) {
    std::promise<int> promise;
    promise.set_value(21);
    int result = co_await await_future(promise.get_future(), workers, state);
    co_return result * 4;
}

coroutine_study::lazy_task<void> exception_path(worker_group& workers, future_state<int>& state) {
    auto fut = std::async(std::launch::async, []() -> int {
        throw std::runtime_error("future failed");
    });
    (void)co_await await_future(std::move(fut), workers, state);
}

} // namespace

int main() {
    using coroutine_study::check;

    worker_group workers;
    future_state<int> ok_state;
    auto task = ok_path(workers, ok_state);
    check(coroutine_study::sync_wait(std::move(task)) == 84, "future awaiter returns 84");
    workers.join();
    check(!ok_state.ready_path && ok_state.suspended, "not-ready future suspends");
    check(ok_state.caller_thread != ok_state.worker_thread, "future waits on a worker thread");
    check(ok_state.worker_thread == ok_state.resume_thread, "coroutine resumes on the worker thread");

    future_state<int> immediate_state;
    auto immediate = ready_path(workers, immediate_state);
    check(coroutine_study::sync_wait(std::move(immediate)) == 84, "ready future returns without suspend");
    check(immediate_state.ready_path && !immediate_state.suspended, "ready future skips await_suspend");

    future_state<int> error_state;
    auto failing = exception_path(workers, error_state);
    bool thrown = false;
    try {
        coroutine_study::sync_wait(std::move(failing));
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    workers.join();
    check(thrown, "future exceptions propagate through await_resume");
    std::cout << "A3_reference OK\n";
}
