#include "mini_ref/mini.hpp"
#include <iostream>

static std::coroutine_handle<> source_handle{};
struct hold_source {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { source_handle = h; }
    void await_resume() const noexcept {}
};
mini_ref::task<int> source(std::shared_ptr<int> keepalive = {}) {
    co_await hold_source{};
    co_return keepalive ? *keepalive : 5;
}
mini_ref::task<void> waiter(mini_ref::shared_task<int> shared, int& out) { out = co_await shared; }
mini_ref::task<void> destroy_later(mini_ref::shared_task<int> shared,
                                  std::optional<mini_ref::task<void>>& later, int& out) {
    out = co_await shared;
    later.reset();
}
void release_source() {
    if (!source_handle) throw std::runtime_error("source did not suspend");
    std::exchange(source_handle, {}).resume();
}
int main() {
    // Cancellation precedes completion. Concurrent destroy/resume is not permitted.
    int abandoned = 0;
    auto shared = mini_ref::share(source());
    { auto gone = waiter(shared, abandoned); gone.start(); }
    release_source();
    if (abandoned != 0) return 1;
    int cached = 0;
    mini_ref::sync_wait(waiter(shared, cached));
    if (cached != 5) return 1;

    int first_value = 0, later_value = 0;
    auto siblings = mini_ref::share(source());
    std::optional<mini_ref::task<void>> later;
    auto first = destroy_later(siblings, later, first_value);
    first.start();
    later.emplace(waiter(siblings, later_value));
    later->start();
    release_source();
    if (first_value != 5 || later_value != 0 || later) return 1;

    std::weak_ptr<int> lifetime;
    {
        auto marker = std::make_shared<int>(7);
        lifetime = marker;
        auto transient = mini_ref::share(source(marker));
        auto gone = waiter(transient, abandoned);
        gone.start();
    }
    if (lifetime.expired()) return 1;
    release_source();
    if (!lifetime.expired() || abandoned != 0) return 1;
    std::cout << "shared_task: abandon, sibling destruction, cached read, owner-free completion checked\n";
}
