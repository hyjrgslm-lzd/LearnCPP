#include "mini_ref/mini.hpp"

#include <cstdlib>

static mini_ref::task<int> scheduled(mini_ref::run_loop& loop) {
    co_await loop.schedule();
    co_return 9;
}

static mini_ref::task<void> finish_scheduled(mini_ref::run_loop& loop, int& value, bool& done) {
    value = co_await scheduled(loop);
    done = true;
}

int main() {
    mini_ref::run_loop loop;
    bool done = false;
    int value = 0;
    auto t = finish_scheduled(loop, value, done);

    t.start();
    if (done) std::abort();
    if (!loop.run_one()) std::abort();
    if (!done || value != 9) std::abort();
}
