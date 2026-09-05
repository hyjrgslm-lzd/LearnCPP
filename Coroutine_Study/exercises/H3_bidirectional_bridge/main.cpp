// H-3 starter: make my_task both a sender and an awaitable.

#include <stdexec/execution.hpp>

#include <coroutine>
#include <cstdio>
#include <exception>

namespace ex = stdexec;

template <class T>
class my_task {
public:
    using sender_concept = ex::sender_tag;

    struct promise_type {
        my_task get_return_object();
        std::suspend_always initial_suspend() noexcept { return {}; }

        // TODO: final_suspend must notify an external receiver when connected,
        // then resume the exact continuation handle when co_awaited.
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T value) { value_ = value; }
        void unhandled_exception() { error_ = std::current_exception(); }

        T value_{};
        std::exception_ptr error_;
    };

    template <class Receiver>
    struct op_state {
        // TODO: own the task frame and receiver; start() resumes the coroutine.
    };

    template <class Receiver>
    op_state<Receiver> connect(Receiver receiver) &&;
};

template <class T>
struct task_awaiter {
    // TODO: expose await_ready/await_suspend/await_resume for my_task<T>.
    // Prefer promise_type::await_transform(my_task<T>&&) over generic
    // await_ready/await_suspend members on my_task itself.
};

int main()
{
    auto r = ex::sync_wait(ex::just(3));
    if (r) {
        auto [v] = *r;
        std::printf("starter smoke: stdexec sender value=%d\n", v);
    }
    std::puts("TODO: complete my_task sender path and await_transform await path.");
    return 0;
}
