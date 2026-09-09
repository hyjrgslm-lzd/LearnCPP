#include <stdexcept>
#include <utility>
#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"

using coroutine_study::check;
using coroutine_study::lazy_task;
using coroutine_study::sync_wait;

int body_runs = 0;
lazy_task<int> value() { ++body_runs; co_return 7; }
lazy_task<int> nested() { co_return co_await value(); }
lazy_task<int> fail() { throw std::runtime_error("boom"); co_return 0; }

int main() {
    check(sync_wait(nested()) == 7, "sync_wait starts root once and waits for final completion");
    check(body_runs == 1, "nested task body runs exactly once");
    auto t = value();
    t.start();
    bool rejected = false;
    try { t.start(); } catch (const std::logic_error&) { rejected = true; }
    check(rejected, "manual second start is rejected");
    bool caught = false;
    try { (void)sync_wait(fail()); } catch (const std::runtime_error&) { caught = true; }
    check(caught, "sync_wait rethrows coroutine exception");
}
