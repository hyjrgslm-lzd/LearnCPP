#include "mini_ref/mini.hpp"

#include <coroutine>
#include <iostream>

static std::coroutine_handle<> source_handle{};

struct hold_source {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { source_handle = h; }
    void await_resume() const noexcept {}
};

mini_ref::task<int> source() {
    co_await hold_source{};
    co_return 5;
}

mini_ref::task<void> waiter(mini_ref::shared_task<int> shared, int& out) {
    out = co_await shared;
}

int main() {
    int out = 0;
    auto shared = mini_ref::share(source());
    {
        auto w = waiter(shared, out);
        w.start();
    }
    if (!source_handle) {
        std::cerr << "source did not suspend\n";
        return 2;
    }
    source_handle.resume();
    std::cout << "out=" << out << "\n";
    return 0;
}
