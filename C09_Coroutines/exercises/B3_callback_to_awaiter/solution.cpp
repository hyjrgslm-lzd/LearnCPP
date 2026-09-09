#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"

#include <chrono>
#include <coroutine>
#include <iostream>
#include <mutex>
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

struct callback_state {
    std::mutex mutex;
    int result = 0;
    std::thread::id caller_thread;
    std::thread::id callback_thread;
};

template <class Callback>
void async_add(worker_group& workers, int a, int b, Callback cb) {
    workers.submit([a, b, cb = std::move(cb)]() mutable {
        std::this_thread::sleep_for(20ms);
        cb(a + b);
    });
}

struct async_add_awaiter {
    int a;
    int b;
    worker_group& workers;
    callback_state& state;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        state.caller_thread = std::this_thread::get_id();
        auto* state_ptr = &state;
        async_add(workers, a, b, [state_ptr, h](int value) {
            {
                std::lock_guard lock(state_ptr->mutex);
                state_ptr->result = value;
                state_ptr->callback_thread = std::this_thread::get_id();
            }
            h.resume();
        });
    }
    int await_resume() const {
        std::lock_guard lock(state.mutex);
        return state.result;
    }
};

coroutine_study::lazy_task<int> compute(worker_group& workers, callback_state& state, int a, int b) {
    int sum = co_await async_add_awaiter{a, b, workers, state};
    co_return sum * 3;
}

} // namespace

int main() {
    worker_group workers;
    callback_state state;
    auto task = compute(workers, state, 7, 5);
    coroutine_study::check(coroutine_study::sync_wait(std::move(task)) == 36, "callback awaiter result");
    workers.join();
    coroutine_study::check(state.caller_thread != state.callback_thread, "callback runs on worker thread");
    std::cout << "B3_reference OK\n";
}
