#include "mini_ref/mini.hpp"

#include <coroutine>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <stop_token>

struct stop_awaiter {
    std::stop_token token;
    std::coroutine_handle<> handle{};
    std::optional<std::stop_callback<std::function<void()>>> callback;

    bool await_ready() const noexcept { return token.stop_requested(); }
    void await_suspend(std::coroutine_handle<> h) {
        handle = h;
        callback.emplace(token, [this] {
            if (handle) handle.resume();
        });
    }
    void await_resume() const noexcept {}
};

mini_ref::task<int> fail_now() {
    throw std::runtime_error("left failed");
    co_return 1;
}

mini_ref::task<int> wait_for_stop(std::stop_token token) {
    co_await stop_awaiter{token};
    co_return 2;
}

int main() {
    std::stop_source source;
    try {
        (void)mini_ref::sync_wait(mini_ref::when_any(fail_now(), wait_for_stop(source.get_token()), source));
        std::cout << "unexpected success\n";
        return 2;
    } catch (const std::runtime_error& e) {
        std::cout << "caught=" << e.what() << "\n";
        return 0;
    }
}
