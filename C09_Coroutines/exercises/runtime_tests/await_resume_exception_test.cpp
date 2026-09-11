#include "coroutine_study/lazy_task.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

struct tracked_value {
    static inline int alive = 0;
    static inline bool fail_move = false;
    int value;
    explicit tracked_value(int n) : value(n) { ++alive; }
    tracked_value(tracked_value&& other) : value(other.value) {
        if (fail_move) throw std::runtime_error("result move failed");
        ++alive;
    }
    ~tracked_value() { --alive; }
};

coroutine_study::lazy_task<tracked_value> child_value() { co_return 7; }
coroutine_study::lazy_task<int> consume_value() {
    auto value = co_await child_value();
    co_return value.value;
}

int main() {
    if (coroutine_study::sync_wait(consume_value()) != 7 || tracked_value::alive != 0) return 1;
    tracked_value::fail_move = true;
    bool caught = false;
    try { (void)coroutine_study::sync_wait(consume_value()); }
    catch (const std::runtime_error& error) { caught = std::string_view(error.what()) == "result move failed"; }
    std::cout << "caught=" << caught << " alive_after_unwind=" << tracked_value::alive << '\n';
    return caught && tracked_value::alive == 0 ? 0 : 1;
}
