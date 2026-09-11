#define main g1_student_original_main
#include "../../../../../exercises/G1_shared_task/main.cpp"
#undef main

#include <coroutine>
#include <cstdio>

struct main_legal_external_suspend {
    static inline std::coroutine_handle<> producer{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { producer = h; }
    void await_resume() const noexcept {}
};

shared_task<int> main_externally_completed_shared_legal() {
    co_await main_legal_external_suspend{};
    co_return 19;
}

lazy_task<void> main_consume_legal(shared_task<int> st, int& out) {
    out = co_await st;
}

int main() {
    int out = 0;
    {
        auto st = main_externally_completed_shared_legal();
        st.resume();
        auto waiter = main_consume_legal(st, out);
        waiter.resume();
        main_legal_external_suspend::producer.resume();
        if (out != 19 || !waiter.done()) return 2;
    }
    std::puts("g1 main explicit-resume legal owner release path checked");
    return 0;
}
