#include <coroutine>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

struct task {
    struct promise_type {
        int value = 0;
        std::coroutine_handle<> continuation;
        task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void return_value(int v) noexcept { value = v; }
        void unhandled_exception() { throw; }
    };
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~task() { if (h_) h_.destroy(); }
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept { h_.promise().continuation = caller; return h_; }
    int await_resume() noexcept { return h_.promise().value; }
    int get() { h_.resume(); return h_.promise().value; }
    std::coroutine_handle<promise_type> h_;
};

task leaf(int depth) { co_return depth; }
task chain(int depth) { if (depth == 0) co_return co_await leaf(0); co_return 1 + co_await chain(depth - 1); }

int main() {
    check(chain(1000).get() == 1000, "handle-return final_suspend transfers through a 1000-deep await chain");
}
