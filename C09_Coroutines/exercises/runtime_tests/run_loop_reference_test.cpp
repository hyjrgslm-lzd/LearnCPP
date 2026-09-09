#include "coroutine_study/runtime.hpp"
#include "test_check.hpp"

#include <vector>

using namespace coroutine_study;

lazy_task<void> hop(run_loop& loop, std::vector<int>& trace) {
    trace.push_back(1);
    co_await loop.schedule();
    trace.push_back(2);
}

int main() {
    run_loop loop;
    std::vector<int> trace;

    auto task = hop(loop, trace);
    task.start();
    check((trace == std::vector<int>{1}));
    check(loop.run_one());
    check((trace == std::vector<int>{1, 2}));
    check(task.done());
}
