#include "mini/when_any.hpp"
#include "manual_event.hpp"
#include "coroutine_study/exercise_check.hpp"
#include <iostream>

mini::task<int> branch(manual_event& event, int value, int& entered) {
    ++entered;
    co_await event;
    co_return value;
}
mini::task<std::variant<int, int>> race(manual_event& a, manual_event& b,
                                      int& entered, std::stop_source stop) {
    co_return co_await mini::when_any(branch(a, 11, entered), branch(b, 22, entered), stop);
}
int main() {
    try {
        manual_event left, right;
        int entered = 0;
        std::stop_source stop;
        auto root = race(left, right, entered, stop);
        root.h_.resume();
        if (root.h_.done()) (void)root.await_resume(); // report the actual TODO exception
        coroutine_study::check(entered == 2 && !root.h_.done(), "when_any must start both children");
        right.set();
        coroutine_study::check(stop.stop_requested() && !root.h_.done(), "winner must cancel and wait for loser");
        left.set();
        coroutine_study::check(root.h_.done(), "when_any did not finish after both children");
        auto result = root.await_resume();
        coroutine_study::check(result.index() == 1 && std::get<1>(result) == 22, "right-first winner lost");
        std::cout << "when_any: fan-out, right winner, stop, loser drain checked\n";
    } catch (const std::exception& e) { std::cerr << "starter check failed: " << e.what() << '\n'; return 1; }
}
