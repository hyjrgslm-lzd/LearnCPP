#include <stdexec/execution.hpp>
#include <coroutine>
#include <iostream>
#include <optional>
#include <exception>
#include <variant>

namespace ex = stdexec;

// ============ my_task<T> ============
template <typename T>
struct my_task {
    struct promise_type {
        std::optional<T> result_;
        std::exception_ptr error_;
        std::coroutine_handle<> continuation_;

        my_task get_return_object() {
            return my_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        // TODO [必做]: final_suspend - resume continuation if set
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                // TODO [必做]: await_suspend should resume continuation_
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    // TODO
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }

        void return_value(T value) {
            result_ = std::move(value);
        }

        void unhandled_exception() {
            error_ = std::current_exception();
        }

        // TODO [必做]: await_transform(Sender) -> convert sender to awaitable
        // This is the key bridge: when user writes co_await some_sender,
        // this transforms it into something the coroutine can suspend on
        template <typename Sender>
        auto await_transform(Sender&& sndr) {
            // TODO [必做]: return a sender_awaitable that:
            // - await_ready() returns false
            // - await_suspend() calls connect(sndr, bridge_receiver) then start
            // - await_resume() returns the value or rethrows error
        }
    };

    std::coroutine_handle<promise_type> handle_;

    explicit my_task(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~my_task() { if (handle_) handle_.destroy(); }

    my_task(my_task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    my_task& operator=(my_task&&) = delete;

    // TODO [必做]: a way to start the task and get the result
    // For simplicity, a blocking run() method:
    T run() {
        handle_.resume(); // start the coroutine (it was initially suspended)
        // After it completes (all co_awaits done, return_value called):
        if (handle_.promise().error_)
            std::rethrow_exception(handle_.promise().error_);
        return std::move(*handle_.promise().result_);
    }
};

// ============ bridge_receiver (used inside await_transform) ============
// TODO [必做]: define a bridge_receiver that:
// - set_value stores result and resumes the coroutine
// - set_error stores exception and resumes the coroutine
// - set_stopped could store a cancellation marker

int main() {
    // TODO [必做]: write a coroutine that co_awaits just(42) and returns the value
    // auto task = []() -> my_task<int> {
    //     int value = co_await ex::just(42);
    //     std::cout << "Got: " << value << "\n";
    //     co_return value + 1;
    // }();
    // int result = task.run();
    // std::cout << "Final: " << result << " (expect 43)\n";

    return 0;
}
