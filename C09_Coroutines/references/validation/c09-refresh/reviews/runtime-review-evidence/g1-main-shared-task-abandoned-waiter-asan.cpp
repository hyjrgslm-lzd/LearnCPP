#define main g1_student_original_main
#include "../../../../../exercises/G1_shared_task/main.cpp"
#undef main

#include <coroutine>

struct main_external_suspend {
    static inline std::coroutine_handle<> producer{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept { producer = h; }
    void await_resume() const noexcept {}
};

shared_task<int> main_externally_completed_shared() {
    co_await main_external_suspend{};
    co_return 17;
}

lazy_task<void> main_consume(shared_task<int> st, int& out) {
    out = co_await st;
}

int main() {
    int out = 0;
    auto st = main_externally_completed_shared();
    st.resume();
    {
        auto waiter = main_consume(st, out);
        waiter.resume();
    }
    main_external_suspend::producer.resume();
    return out;
}
