#pragma once

#include <stdexec/execution.hpp>

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

namespace ex = stdexec;

template <class T>
class my_task;

template <class T>
struct task_awaiter;

template <class Sender>
struct sender_awaiter;

template <class T>
class my_task {
public:
    using sender_concept = ex::sender_tag;

    struct promise_type {
        my_task get_return_object();
        std::suspend_always initial_suspend() noexcept { return {}; }

        // TODO: final_suspend must notify a connected receiver or resume the
        // exact continuation saved by task_awaiter.
        std::suspend_always final_suspend() noexcept { return {}; }

        template <class U>
        task_awaiter<U> await_transform(my_task<U>&& task) noexcept
        {
            return task_awaiter<U>{std::move(task)};
        }

        template <class Sender>
            requires ex::sender<std::remove_cvref_t<Sender>>
        sender_awaiter<std::remove_cvref_t<Sender>> await_transform(Sender&& sender)
        {
            return sender_awaiter<std::remove_cvref_t<Sender>>{std::forward<Sender>(sender)};
        }

        void return_value(T value) { value_ = value; }
        void unhandled_exception() { error_ = std::current_exception(); }

        T value_{};
        std::exception_ptr error_;
    };

    template <class Receiver>
    struct op_state {
        // TODO: own the task frame and receiver; start() should resume the
        // coroutine and let final_suspend publish value/error/stopped exactly once.
        my_task task;
        Receiver receiver;

        void start() noexcept
        {
            task.handle.resume();
            ex::set_value(std::move(receiver), task.handle.promise().value_);
        }
    };

    template <class Receiver>
    op_state<Receiver> connect(Receiver receiver) && { return {std::move(*this), std::move(receiver)}; }

    explicit my_task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    my_task(my_task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    my_task(const my_task&) = delete;
    ~my_task()
    {
        if (handle) handle.destroy();
    }

    std::coroutine_handle<promise_type> handle;
};

template <class T>
struct task_awaiter {
    // TODO: resume task, suspend parent, and return child's actual value/error.
    my_task<T> task;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    T await_resume() { return {}; }
};

template <class Sender>
struct sender_awaiter {
    // TODO: connect/start sender and resume only after sender completion.
    Sender sender;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int await_resume() { return 0; }
};

template <class T>
my_task<T> my_task<T>::promise_type::get_return_object()
{
    return my_task{std::coroutine_handle<promise_type>::from_promise(*this)};
}
