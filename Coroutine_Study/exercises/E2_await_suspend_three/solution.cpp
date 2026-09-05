#include <coroutine>
#include <string>
#include <vector>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

std::vector<std::string> events;

struct void_suspend { bool await_ready() noexcept { return false; } void await_suspend(std::coroutine_handle<> h) noexcept { events.push_back("void suspend"); h.resume(); } void await_resume() noexcept { events.push_back("void resume"); } };
struct bool_suspend { bool again; bool await_ready() noexcept { return false; } bool await_suspend(std::coroutine_handle<>) noexcept { events.push_back(again ? "bool true" : "bool false"); return again; } void await_resume() noexcept { events.push_back("bool resume"); } };
struct handle_suspend { bool await_ready() noexcept { return false; } std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept { events.push_back("handle noop"); return std::noop_coroutine(); } void await_resume() noexcept { events.push_back("handle resume"); } };

struct task { struct promise_type { task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; } std::suspend_always initial_suspend() noexcept { return {}; } std::suspend_always final_suspend() noexcept { return {}; } void return_void() noexcept {} void unhandled_exception() { throw; } }; explicit task(std::coroutine_handle<promise_type> h): h_(h) {} ~task(){ if(h_) h_.destroy(); } void resume(){ if(!h_.done()) h_.resume(); } bool done() const { return h_.done(); } std::coroutine_handle<promise_type> h_; };

task probe() { co_await void_suspend{}; co_await bool_suspend{false}; co_await bool_suspend{true}; co_await handle_suspend{}; }

int main() {
    auto t = probe();
    t.resume();
    check(!t.done(), "bool true keeps coroutine suspended");
    t.resume();
    t.resume();
    check(t.done(), "caller can resume after real suspension");
    check(events == std::vector<std::string>({"void suspend", "void resume", "bool false", "bool resume", "bool true", "bool resume", "handle noop", "handle resume"}), "void/bool/handle await_suspend semantics observed");
}
