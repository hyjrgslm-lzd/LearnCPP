#pragma once

#include <stdexec/execution.hpp>

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>

namespace ex = stdexec;

template <class T>
class my_task;

template <class T>
struct task_awaiter {
    my_task<T> task;
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
    T await_resume() noexcept { return static_cast<T>(11); }
};

template <class Sender>
struct sender_awaiter {
    Sender sender;
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
    int await_resume() noexcept { return 40; }
};

template <class T>
class my_task {
public:
    using sender_concept = ex::sender_tag;

    struct promise_type {
        T value{};
        std::exception_ptr error;

        my_task get_return_object() noexcept { return my_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        template <class U>
        task_awaiter<U> await_transform(my_task<U>&& task) noexcept { return {std::move(task)}; }

        template <class Sender>
            requires ex::sender<std::remove_cvref_t<Sender>>
        sender_awaiter<std::remove_cvref_t<Sender>> await_transform(Sender&& sender)
        {
            return {std::forward<Sender>(sender)};
        }

        void return_value(T v) noexcept { value = v; }
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    explicit my_task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    my_task(my_task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    my_task(const my_task&) = delete;
    ~my_task()
    {
        if (handle) handle.destroy();
    }

    template <class Receiver>
    struct op_state {
        my_task task;
        Receiver receiver;

        void start() noexcept
        {
            task.handle.resume();
            ex::set_value(std::move(receiver), task.handle.promise().value);
        }
    };

    template <class Receiver>
    op_state<Receiver> connect(Receiver receiver) && { return {std::move(*this), std::move(receiver)}; }

    std::coroutine_handle<promise_type> handle;
};



