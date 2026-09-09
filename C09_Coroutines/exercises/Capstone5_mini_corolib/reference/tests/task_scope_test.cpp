#include "mini_ref/mini.hpp"

#include <atomic>
#include <cstdlib>

static mini_ref::task<void> inc(std::atomic<int>& count) {
    ++count;
    co_return;
}

int main() {
    std::atomic<int> count{0};
    {
        mini_ref::task_scope scope;
        for (int i = 0; i < 4; ++i) scope.spawn(inc(count));
        scope.wait_empty();
        if (scope.in_flight() != 0) std::abort();
    }
    if (count != 4) std::abort();
}
