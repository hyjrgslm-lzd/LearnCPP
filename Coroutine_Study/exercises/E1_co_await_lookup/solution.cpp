#include <coroutine>
#include <string>
#include <utility>
#include <vector>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

std::vector<std::string> events;

struct plain_awaitable { int v; bool await_ready() noexcept { return true; } void await_suspend(std::coroutine_handle<>) noexcept {} int await_resume() noexcept { return v; } };
struct member_source { int v; plain_awaitable operator co_await() noexcept { events.push_back("member"); return {v}; } };
struct free_source { int v; };
plain_awaitable operator co_await(free_source s) noexcept { events.push_back("free"); return {s.v}; }

struct task {
    struct promise_type {
        int value = 0;
        task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        plain_awaitable await_transform(int v) noexcept { events.push_back("transform"); return {v}; }
        template <class T> T await_transform(T x) noexcept { return x; }
        void return_value(int v) noexcept { value = v; }
        void unhandled_exception() { throw; }
    };
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    ~task() { if (h_) h_.destroy(); }
    int get() { h_.resume(); return h_.promise().value; }
    std::coroutine_handle<promise_type> h_;
};

task probe() {
    int a = co_await 1;
    int b = co_await member_source{2};
    int c = co_await free_source{3};
    co_return a + b + c;
}

int main() {
    check(probe().get() == 6, "co_await conversion result used");
    check(events == std::vector<std::string>({"transform", "member", "free"}), "lookup order: await_transform first, then member/free operator co_await");
}
