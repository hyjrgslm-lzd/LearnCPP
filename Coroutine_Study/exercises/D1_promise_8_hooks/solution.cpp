#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

template <class T>
struct lazy_task {
    struct promise_type {
        std::optional<T> value;
        std::exception_ptr error;
        std::coroutine_handle<> continuation;

        lazy_task get_return_object() noexcept { return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void return_value(T v) noexcept { value.emplace(std::move(v)); }
        void unhandled_exception() noexcept { error = std::current_exception(); }
        static void* operator new(std::size_t n) { return ::operator new(n); }
        static void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
    };

    explicit lazy_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task(const lazy_task&) = delete;
    ~lazy_task() { if (h_) h_.destroy(); }

    T get() {
        check(h_ && !started_, "task must be live and started once");
        started_ = true;
        h_.resume();
        auto& p = h_.promise();
        if (p.error) std::rethrow_exception(p.error);
        check(p.value.has_value(), "co_return must publish a value before final suspend");
        return std::move(*p.value);
    }

    bool await_ready() const noexcept { return !h_ || h_.done(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.error) std::rethrow_exception(p.error);
        return std::move(*p.value);
    }
private:
    std::coroutine_handle<promise_type> h_{};
    bool started_ = false;
};

lazy_task<int> value_task() { co_return 30; }
lazy_task<int> fail_task() { throw std::runtime_error("boom"); co_return 0; }
lazy_task<int> inner() { co_return 42; }
lazy_task<int> outer() { co_return (co_await inner()) * 2; }

int main() {
    check(value_task().get() == 30, "return_value stores result");
    check(outer().get() == 84, "final_suspend resumes continuation");
    bool caught = false;
    try { (void)fail_task().get(); } catch (const std::runtime_error&) { caught = true; }
    check(caught, "unhandled_exception stores exception_ptr for caller");
}
