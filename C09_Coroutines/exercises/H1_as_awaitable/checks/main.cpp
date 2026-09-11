#include "fixture.hpp"
#include "student.hpp"

#include <stdexec/execution.hpp>

#include <coroutine>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ex = stdexec;

struct h1_values {
    int first = -999;
    int second = -999;
};

struct task {
    struct promise_type {
        h1_values value;
        std::exception_ptr error;

        task get_return_object() noexcept
        {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(h1_values v) noexcept { value = v; }
        void unhandled_exception() noexcept { error = std::current_exception(); }

        template <class Sender>
            requires ex::sender<std::remove_cvref_t<Sender>>
        auto await_transform(Sender&& sender)
        {
            return sender_awaitable<std::remove_cvref_t<Sender>>{std::forward<Sender>(sender)};
        }
    };

    explicit task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    task(task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    task(const task&) = delete;
    ~task()
    {
        if (handle) handle.destroy();
    }

    h1_values run()
    {
        handle.resume();
        if (handle.promise().error) std::rethrow_exception(handle.promise().error);
        return handle.promise().value;
    }

    std::coroutine_handle<promise_type> handle;
};

task value_check(int a, int b, h1_fixture& fixture)
{
    int x = co_await fixture.sender(ex::just(a));
    int y = co_await fixture.sender(ex::just(b));
    co_return h1_values{x, y};
}

bool run_case(int a, int b)
{
    h1_fixture fixture;
    const auto value = value_check(a, b, fixture).run();
    const bool ok = value.first == a && value.second == b && fixture.saw_exactly(2, 2);
    if (!ok) {
        std::printf(
            "student check failed: inputs=%d/%d values=%d/%d started=%d completed=%d, expected values match inputs and trace 2/2\n",
            a,
            b,
            value.first,
            value.second,
            fixture.started(),
            fixture.completed());
    }
    return ok;
}

int main()
{
    if (!run_case(19, 23) || !run_case(21, 4)) {
        return 1;
    }
    std::puts("H1 student check passed.");
}
