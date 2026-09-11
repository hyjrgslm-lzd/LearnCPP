#define main g1_solution_original_main
#include "../../../../../exercises/G1_shared_task/solution.cpp"
#undef main

#include <coroutine>
#include <cstdio>

struct legal_external_suspend {
    static inline std::coroutine_handle<> producer{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { producer = h; }
    void await_resume() const noexcept {}
};

shared_task<int> externally_completed_shared_legal() {
    co_await legal_external_suspend{};
    co_return 11;
}

shared_task<int> immediately_completed_shared_legal() {
    co_return 13;
}

void_task consume_legal(shared_task<int> st, int& out) {
    out = co_await st;
}

void_task consume_temporary_legal(int& out) {
    out = co_await immediately_completed_shared_legal();
}

int main() {
    int out = 0;
    {
        auto st = externally_completed_shared_legal();
        auto waiter = consume_legal(st, out);
        waiter.start();
        legal_external_suspend::producer.resume();
        if (out != 11 || !waiter.done()) return 2;
    }
    int temp = 0;
    {
        auto waiter = consume_temporary_legal(temp);
        waiter.start();
        if (temp != 13 || !waiter.done()) return 3;
    }
    std::puts("g1 legal owner release paths checked");
    return 0;
}
