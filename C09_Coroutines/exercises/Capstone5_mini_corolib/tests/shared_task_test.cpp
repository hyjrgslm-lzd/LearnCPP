#include "mini/shared_task.hpp"
#include "mini/task.hpp"
#include "manual_event.hpp"
#include "coroutine_study/exercise_check.hpp"
#include <iostream>

mini::shared_task<int> shared(manual_event& ready, int& starts) {
    ++starts;
    co_await ready;
    co_return 17;
}
mini::task<int> read(mini::shared_task<int> value) { co_return co_await value; }
int main() {
    try {
        manual_event ready;
        int starts = 0;
        auto value = shared(ready, starts);
        auto first = read(value);
        auto second = read(value);
        first.h_.resume();
        if (first.h_.done()) (void)first.await_resume();
        second.h_.resume();
        if (second.h_.done()) (void)second.await_resume();
        coroutine_study::check(starts == 1 && !first.h_.done() && !second.h_.done(), "shared producer must start once");
        ready.set();
        coroutine_study::check(first.h_.done() && second.h_.done(), "all shared waiters must resume");
        coroutine_study::check(first.await_resume() == 17 && second.await_resume() == 17, "shared result mismatch");
        auto late = read(value);
        late.h_.resume();
        coroutine_study::check(late.h_.done() && late.await_resume() == 17 && starts == 1, "late waiter must use cached result");
        std::cout << "shared_task: two waiters, one producer, late cached read checked\n";
    } catch (const std::exception& e) { std::cerr << "starter check failed: " << e.what() << '\n'; return 1; }
}
