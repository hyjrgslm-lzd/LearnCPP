#include "mini_ref/mini.hpp"
#include "async_test_helpers.hpp"

#include <cstdlib>
#include <iostream>
#include <latch>
#include <stdexcept>
#include <stop_token>
#include <variant>

static mini_ref::task<int> fail_after(mini_ref_test::async_threads& threads, std::latch& started, std::latch& release) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    throw std::runtime_error{"first error"};
    co_return 0;
}

static mini_ref::task<int> succeed_after(mini_ref_test::async_threads& threads, std::latch& started, std::latch& release) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    co_return 9;
}

int main() {
    mini_ref_test::async_threads threads;
    std::latch fail_started{1};
    std::latch success_started{1};
    std::latch fail_release{1};
    std::latch success_release{1};
    std::stop_source stop;

    auto parent = [&]() -> mini_ref::task<int> {
        auto v = co_await mini_ref::when_any(fail_after(threads, fail_started, fail_release),
                                             succeed_after(threads, success_started, success_release),
                                             stop);
        if (v.index() != 1) co_return -1;
        co_return std::get<1>(v);
    };

    std::optional<std::tuple<int>> result;
    std::latch done{1};
    threads.spawn([&] {
        result = mini_ref::sync_wait(parent()).value();
        done.count_down();
    });

    fail_started.wait();
    success_started.wait();
    fail_release.count_down();
    success_release.count_down();
    done.wait();
    threads.join_all();

    int value = std::get<0>(*result);
    std::cout << "value=" << value << " stop_requested=" << stop.stop_requested() << "\n";
    return value == 9 ? 0 : 1;
}
