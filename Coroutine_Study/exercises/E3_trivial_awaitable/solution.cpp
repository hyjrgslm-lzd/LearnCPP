#include <coroutine>
#include <exception>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

int suspend_calls = 0;
struct ready_awaitable { bool await_ready() noexcept { return true; } void await_suspend(std::coroutine_handle<>) noexcept { ++suspend_calls; } int await_resume() noexcept { return 7; } };
struct task { struct promise_type { int value=0; task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; } std::suspend_always initial_suspend() noexcept { return {}; } std::suspend_always final_suspend() noexcept { return {}; } void return_value(int v) noexcept { value=v; } void unhandled_exception() noexcept { std::terminate(); } }; explicit task(std::coroutine_handle<promise_type> h): h_(h){} ~task(){ if(h_) h_.destroy(); } int get(){ h_.resume(); return h_.promise().value; } std::coroutine_handle<promise_type> h_; };
task probe() { co_return co_await ready_awaitable{}; }
int main() { check(probe().get() == 7, "await_resume returns value"); check(suspend_calls == 0, "await_ready true skips await_suspend entirely"); }
