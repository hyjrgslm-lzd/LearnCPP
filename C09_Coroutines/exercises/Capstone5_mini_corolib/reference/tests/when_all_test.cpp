#include "mini_ref/mini.hpp"
#include "async_test_helpers.hpp"

#include <atomic>
#include <barrier>
#include <stdexcept>
#include <cstdlib>
#include <latch>
#include <optional>
#include <type_traits>

static mini_ref::task<int> add(int a, int b) {
    co_return a + b;
}

static mini_ref::task<int> delayed(mini_ref_test::async_threads& threads,
                                   std::latch& started,
                                   std::latch& release,
                                   int v) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    co_return v;
}

static mini_ref::task<int> before_suspend_returns(mini_ref_test::async_threads& threads, int v) {
    co_await mini_ref_test::resume_before_suspend_returns{threads};
    co_return v;
}

static mini_ref::task<int> simultaneous(mini_ref_test::async_threads& threads,
                                        std::latch& started,
                                        std::barrier<>& finish,
                                        int v) {
    co_await mini_ref_test::resume_on_barrier{threads, started, finish};
    co_return v;
}

static mini_ref::task<int> fail() {
    throw std::runtime_error{"fail"};
    co_return 0;
}

static mini_ref::task<int> delayed_fail(mini_ref_test::async_threads& threads,
                                        std::latch& started,
                                        std::latch& release) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    throw std::runtime_error{"late fail"};
    co_return 0;
}

static mini_ref::task<int> parent_resume_counter(mini_ref_test::async_threads& threads,
                                                 std::latch& started,
                                                 std::barrier<>& finish,
                                                 std::atomic<int>& resumes) {
    auto both = co_await mini_ref::when_all(simultaneous(threads, started, finish, 3),
                                           simultaneous(threads, started, finish, 4));
    ++resumes;
    co_return std::get<0>(both) + std::get<1>(both);
}

int main() {
    using expected = mini_ref::completion_signatures<std::tuple<int, int>>;
    static_assert(std::is_same_v<
                  mini_ref::when_all_result<mini_ref::task<int>, mini_ref::task<int>>::completion_signatures,
                  expected>);

    auto result = mini_ref::sync_wait(mini_ref::when_all(add(2, 3), add(2, 4)));
    auto [sum, product] = std::get<0>(*result);
    if (sum != 5 || product != 6) std::abort();

    mini_ref_test::async_threads threads;
    auto early = mini_ref::sync_wait(mini_ref::when_all(before_suspend_returns(threads, 7),
                                                       before_suspend_returns(threads, 8)));
    auto [early_a, early_b] = std::get<0>(*early);
    if (early_a != 7 || early_b != 8) std::abort();

    std::latch started{2};
    std::latch release{1};
    std::latch delayed_done{1};
    std::optional<std::tuple<std::tuple<int, int>>> delayed_result;
    threads.spawn([&] {
        delayed_result = mini_ref::sync_wait(
            mini_ref::when_all(delayed(threads, started, release, 1),
                               delayed(threads, started, release, 2))).value();
        delayed_done.count_down();
    });
    started.wait();
    release.count_down();
    delayed_done.wait();
    auto [a, b] = std::get<0>(*delayed_result);
    if (a != 1 || b != 2) std::abort();

    std::latch simultaneous_started{2};
    std::barrier simultaneous_finish{3};
    std::latch simultaneous_done{1};
    std::atomic<int> resumes{0};
    std::optional<std::tuple<int>> simultaneous_result;
    threads.spawn([&] {
        simultaneous_result = mini_ref::sync_wait(
            parent_resume_counter(threads, simultaneous_started, simultaneous_finish, resumes)).value();
        simultaneous_done.count_down();
    });
    simultaneous_started.wait();
    simultaneous_finish.arrive_and_wait();
    simultaneous_done.wait();
    if (std::get<0>(*simultaneous_result) != 7 || resumes != 1) std::abort();

    bool caught = false;
    try {
        (void)mini_ref::sync_wait(mini_ref::when_all(fail(), add(1, 1)));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    if (!caught) std::abort();

    std::latch fail_started{2};
    std::latch fail_release{1};
    std::latch fail_done{1};
    caught = false;
    threads.spawn([&] {
        try {
            (void)mini_ref::sync_wait(mini_ref::when_all(delayed_fail(threads, fail_started, fail_release),
                                                        delayed(threads, fail_started, fail_release, 9)));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        fail_done.count_down();
    });
    fail_started.wait();
    fail_release.count_down();
    fail_done.wait();
    if (!caught) std::abort();

    threads.join_all();
}
