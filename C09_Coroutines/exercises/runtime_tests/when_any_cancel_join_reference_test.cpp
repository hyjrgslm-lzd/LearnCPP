#include "coroutine_study/runtime.hpp"
#include "test_check.hpp"

#include <atomic>
#include <chrono>
#include <stop_token>
#include <thread>

using namespace coroutine_study;
using namespace std::chrono_literals;

lazy_task<int> delayed(std::stop_token token, int value, int delay_ms, std::atomic<int>& cancelled) {
    for (int i = 0; i < delay_ms; ++i) {
        if (token.stop_requested()) {
            ++cancelled;
            co_return -value;
        }
        std::this_thread::sleep_for(1ms);
    }
    co_return value;
}

int main() {
    std::stop_source stop;
    std::atomic<int> cancelled = 0;
    auto result = when_any_cancel_join(
        stop,
        delayed(stop.get_token(), 1, 5, cancelled),
        delayed(stop.get_token(), 2, 100, cancelled)
    );
    check(result.index == 0);
    check(result.value == 1);
    check(stop.stop_requested());
    check(cancelled == 1);
}
