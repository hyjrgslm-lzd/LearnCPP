#include "mini_ref/mini.hpp"
#include "async_test_helpers.hpp"

#include <atomic>
#include <barrier>
#include <cstdlib>
#include <latch>
#include <optional>
#include <stdexcept>
#include <stop_token>

static mini_ref::task<int> value(int v) {
    co_return v;
}

static mini_ref::task<int> delayed(mini_ref_test::async_threads& threads,
                                   std::latch& started,
                                   std::latch& release,
                                   int v,
                                   mini_ref::stop_token token,
                                   std::atomic<bool>& loser_seen_cancel) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    if (token.stop_requested()) {
        loser_seen_cancel = true;
        co_return -1;
    }
    co_return v;
}

static mini_ref::task<int> before_suspend_returns(mini_ref_test::async_threads& threads, int v) {
    co_await mini_ref_test::resume_before_suspend_returns{threads};
    co_return v;
}

static mini_ref::task<int> delayed_fail(mini_ref_test::async_threads& threads,
                                        std::latch& started,
                                        std::latch& release) {
    co_await mini_ref_test::resume_after_release{threads, started, release};
    throw std::runtime_error{"delayed fail"};
    co_return 0;
}

static mini_ref::task<int> simultaneous_value(mini_ref_test::async_threads& threads,
                                              std::latch& started,
                                              std::barrier<>& finish,
                                              int v) {
    co_await mini_ref_test::resume_on_barrier{threads, started, finish};
    co_return v;
}

static mini_ref::task<int> parent_resume_counter(mini_ref_test::async_threads& threads,
                                                 std::latch& started,
                                                 std::barrier<>& finish,
                                                 std::stop_source stop,
                                                 std::atomic<int>& resumes) {
    auto winner = co_await mini_ref::when_any(simultaneous_value(threads, started, finish, 1),
                                             simultaneous_value(threads, started, finish, 2),
                                             stop);
    ++resumes;
    co_return static_cast<int>(winner.index());
}

int main() {
    auto result = mini_ref::sync_wait(mini_ref::when_any(value(11), value(22)));
    auto winner = std::get<0>(*result);
    if (!((winner.index() == 0 && std::get<0>(winner) == 11) ||
          (winner.index() == 1 && std::get<1>(winner) == 22))) std::abort();

    mini_ref_test::async_threads threads;
    auto early = mini_ref::sync_wait(mini_ref::when_any(before_suspend_returns(threads, 31),
                                                       before_suspend_returns(threads, 32)));
    auto early_winner = std::get<0>(*early);
    if (!((early_winner.index() == 0 && std::get<0>(early_winner) == 31) ||
          (early_winner.index() == 1 && std::get<1>(early_winner) == 32))) std::abort();

    std::latch simultaneous_started{2};
    std::barrier simultaneous_finish{3};
    std::latch simultaneous_done{1};
    std::atomic<int> resumes{0};
    mini_ref::stop_source simultaneous_stop;
    std::optional<std::tuple<int>> simultaneous;
    threads.spawn([&] {
        simultaneous = mini_ref::sync_wait(parent_resume_counter(threads,
                                                                 simultaneous_started,
                                                                 simultaneous_finish,
                                                                 simultaneous_stop,
                                                                 resumes)).value();
        simultaneous_done.count_down();
    });
    simultaneous_started.wait();
    simultaneous_finish.arrive_and_wait();
    simultaneous_done.wait();
    if (!simultaneous || resumes != 1) std::abort();

    mini_ref::stop_source stop;
    std::atomic<bool> loser_seen_cancel{false};
    std::latch left_started{1};
    std::latch right_started{1};
    std::latch left_release{1};
    std::latch right_release{1};
    std::latch reversed_done{1};
    std::latch winner_chosen{1};
    std::stop_callback winner_callback{stop.get_token(), [&] { winner_chosen.count_down(); }};
    std::optional<std::tuple<std::variant<int, int>>> reversed;
    threads.spawn([&] {
        reversed = mini_ref::sync_wait(
            mini_ref::when_any(delayed(threads, left_started, left_release, 1, stop.get_token(), loser_seen_cancel),
                               delayed(threads, right_started, right_release, 2, stop.get_token(), loser_seen_cancel),
                               stop)).value();
        reversed_done.count_down();
    });
    left_started.wait();
    right_started.wait();
    right_release.count_down();
    winner_chosen.wait();
    if (reversed_done.try_wait()) std::abort();
    left_release.count_down();
    reversed_done.wait();
    auto reversed_winner = std::get<0>(*reversed);
    if (reversed_winner.index() != 1 || std::get<1>(reversed_winner) != 2) std::abort();
    if (!loser_seen_cancel) std::abort();

    std::latch race_started{2};
    std::latch race_release{1};
    std::latch race_done{1};
    std::optional<std::tuple<std::variant<int, int>>> race;
    threads.spawn([&] {
        race = mini_ref::sync_wait(mini_ref::when_any(delayed_fail(threads, race_started, race_release),
                                                     delayed(threads, race_started, race_release, 5,
                                                             mini_ref::stop_token{}, loser_seen_cancel))).value();
        race_done.count_down();
    });
    race_started.wait();
    race_release.count_down();
    race_done.wait();
    auto race_winner = std::get<0>(*race);
    if (race_winner.index() != 1 || std::get<1>(race_winner) != 5) std::abort();

    std::latch both_fail_started{2};
    std::latch both_fail_release{1};
    std::latch both_fail_done{1};
    bool caught = false;
    threads.spawn([&] {
        try {
            (void)mini_ref::sync_wait(mini_ref::when_any(delayed_fail(threads, both_fail_started, both_fail_release),
                                                        delayed_fail(threads, both_fail_started, both_fail_release)));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        both_fail_done.count_down();
    });
    both_fail_started.wait();
    both_fail_release.count_down();
    both_fail_done.wait();
    if (!caught) std::abort();

    threads.join_all();
}
