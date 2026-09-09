#include "coroutine_study/lazy_task.hpp"
#include "test_check.hpp"

#include <new>

using namespace coroutine_study;

lazy_task<int> allocated_task() {
    co_return 3;
}

int main() {
    lazy_task_last_allocation_size = 0;
    check(sync_wait(allocated_task()) == 3);
    check(lazy_task_last_allocation_size > 0);

    lazy_task_fail_next_allocation = true;
    auto failed = allocated_task();
    check(!failed.valid());

    bool threw = false;
    try {
        (void)sync_wait(std::move(failed));
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    check(threw);
}
