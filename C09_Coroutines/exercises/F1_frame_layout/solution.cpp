#include <coroutine>
#include <string>
#include <utility>
#include <vector>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

std::vector<std::string> events;
struct marker { std::string name; explicit marker(std::string n): name(std::move(n)) { events.push_back("construct " + name); } ~marker(){ events.push_back("destroy " + name); } };
struct task { struct promise_type { task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; } std::suspend_always initial_suspend() noexcept { events.push_back("initial suspend"); return {}; } std::suspend_always final_suspend() noexcept { events.push_back("final suspend"); return {}; } void return_void() noexcept { events.push_back("return void"); } void unhandled_exception(){ throw; } }; explicit task(std::coroutine_handle<promise_type> h): h_(h){} ~task(){ if(h_) h_.destroy(); } void resume(){ if(!h_.done()) h_.resume(); } bool done() const { return h_.done(); } std::coroutine_handle<promise_type> h_; };

task probe() { marker before{"before"}; co_await std::suspend_always{}; marker after{"after"}; co_return; }

int main() {
    auto t = probe();
    check(events == std::vector<std::string>({"initial suspend"}), "locals are not constructed before first resume");
    t.resume();
    check(events.back() == "construct before", "local live across first suspension");
    t.resume();
    check(t.done(), "second resume reaches final suspend");
    check(events == std::vector<std::string>({"initial suspend", "construct before", "construct after", "return void", "destroy after", "destroy before", "final suspend"}), "observable lifetime order, not physical frame layout");
}
