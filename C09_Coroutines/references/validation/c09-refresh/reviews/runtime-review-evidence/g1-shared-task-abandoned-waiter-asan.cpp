#define main g1_solution_original_main
#include "../../../../../exercises/G1_shared_task/solution.cpp"
#undef main

#include <coroutine>

struct external_suspend {
    static inline std::coroutine_handle<> producer{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { producer = h; }
    void await_resume() const noexcept {}
};

shared_task<int> externally_completed_shared() {
    co_await external_suspend{};
    co_return 7;
}

void_task consume(shared_task<int> st, int& out) {
    out = co_await st;
}

int main() {
    int out = 0;
    auto st = externally_completed_shared();
    {
        auto waiter = consume(st, out);
        waiter.start();
    }
    external_suspend::producer.resume();
    return out;
}
