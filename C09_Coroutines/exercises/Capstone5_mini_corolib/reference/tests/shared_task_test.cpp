#include "mini_ref/mini.hpp"

#include <cstdlib>
#include <chrono>
#include <thread>

static mini_ref::task<int> source() {
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
    co_return 5;
}

static mini_ref::task<int> use_twice(mini_ref::shared_task<int> shared) {
    int a = co_await shared;
    int b = co_await shared;
    co_return a + b;
}

int main() {
    auto result = mini_ref::sync_wait(use_twice(mini_ref::share(source())));
    if (!result || std::get<0>(*result) != 10) std::abort();

    auto shared = mini_ref::share(source());
    auto first = [shared]() -> mini_ref::task<int> { co_return co_await shared; };
    auto second = [shared]() -> mini_ref::task<int> { co_return co_await shared; };
    auto both = mini_ref::sync_wait(mini_ref::when_all(first(), second()));
    auto [a, b] = std::get<0>(*both);
    if (a != 5 || b != 5) std::abort();
}
