#include "coroutine_study/runtime.hpp"
#include "test_check.hpp"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace coroutine_study;
using namespace std::chrono_literals;

lazy_task<void> worker(std::stop_token token, std::atomic<int>& ticks) {
    while (!token.stop_requested()) {
        ++ticks;
        std::this_thread::sleep_for(1ms);
    }
    co_return;
}

lazy_task<void> failing() {
    throw std::runtime_error("scope failed");
    co_return;
}

int main() {
    std::atomic<int> ticks = 0;
    {
        task_scope scope;
        scope.spawn(worker(scope.get_token(), ticks));
        std::this_thread::sleep_for(10ms);
        scope.request_stop();
        scope.join();
    }
    check(ticks > 0);

    bool threw = false;
    try {
        task_scope scope;
        scope.spawn(failing());
        scope.join();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw);
}
