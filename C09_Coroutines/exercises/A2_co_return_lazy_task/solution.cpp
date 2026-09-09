#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"

#include <iostream>
#include <vector>

namespace {

std::vector<int> trace;

coroutine_study::lazy_task<int> compute_async(int x) {
    trace.push_back(1);
    int step1 = x + 10;
    trace.push_back(2);
    int step2 = step1 * 2;
    trace.push_back(3);
    co_return step2 - 5;
}

int compute_sync(int x) {
    int step1 = x + 10;
    int step2 = step1 * 2;
    return step2 - 5;
}

} // namespace

int main() {
    using coroutine_study::check;

    auto task = compute_async(5);
    check(trace.empty(), "lazy_task body must not run at construction");
    check(compute_sync(5) == 25, "sync baseline");
    check(coroutine_study::sync_wait(std::move(task)) == 25, "co_return result");
    check((trace == std::vector<int>{1, 2, 3}), "body order");

    std::cout << "A2_reference OK\n";
}
