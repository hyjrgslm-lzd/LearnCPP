#include "mini_ref/mini.hpp"

#include <cstdlib>

static mini_ref::task<int> scheduled(mini_ref::run_loop& loop) {
    co_await loop.schedule();
    co_return 9;
}

int main() {
    mini_ref::run_loop loop;
    bool done = false;
    int value = 0;
    auto t = [&]() -> mini_ref::task<void> {
        value = co_await scheduled(loop);
        done = true;
    }();

    t.start();
    if (done) std::abort();
    if (!loop.run_one()) std::abort();
    if (!done || value != 9) std::abort();
}
