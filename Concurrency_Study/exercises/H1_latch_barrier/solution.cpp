#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <atomic>
#include <barrier>
#include <future>
#include <iostream>
#include <latch>
#include <numeric>
#include <vector>

void latch_start() {
    std::latch ready(4), go(1);
    cs::check(!ready.try_wait(), "positive latch cannot report completion");
    std::array<int, 4> prepared{};
    std::atomic<int> started{0};
    std::vector<std::future<void>> workers;
    workers.reserve(4);
    try {
        for (int i = 0; i < 4; ++i)
            workers.push_back(std::async(std::launch::async, [&, i] {
                prepared[i] = i + 1;
                ready.count_down();
                go.wait();
                ++started;
            }));
    } catch (...) { go.count_down(); throw; }
    ready.wait();
    const bool prepared_before_start = started == 0 && std::accumulate(prepared.begin(), prepared.end(), 0) == 10;
    go.count_down();
    for (auto& worker : workers) worker.get();
    cs::check(prepared_before_start && started == 4, "all preparation before release");
    // try_wait is allowed to return false spuriously; never require true.
    ready.wait();
    go.wait(); // repeat wait is legal; repeat count_down(1) is not
}

void barrier_phases() {
    constexpr int count = 3, rounds = 4;
    std::array<int, count> slots{};
    std::array<int, rounds> totals{};
    int phase = 0;
    auto finish = [&]() noexcept {
        totals[phase++] = std::accumulate(slots.begin(), slots.end(), 0);
    };
    std::barrier barrier(count, finish);
    std::promise<bool> launch;
    auto start = launch.get_future().share();
    std::vector<std::future<void>> workers;
    workers.reserve(count);
    try {
        for (int i = 0; i < count; ++i)
            workers.push_back(std::async(std::launch::async, [&, i, start] {
                if (!start.get()) return;
                for (int round = 0; round < rounds; ++round) {
                    slots[i] = (round + 1) * 10 + i;
                    // Only local work may occur between arrive and wait.
                    auto arrival = barrier.arrive();
                    barrier.wait(std::move(arrival));
                }
            }));
    } catch (...) { launch.set_value(false); throw; }
    launch.set_value(true);
    for (auto& worker : workers) worker.get();
    cs::check(phase == rounds, "completion exactly once for each waited phase");
    for (int r = 0; r < rounds; ++r)
        cs::check(totals[r] == 30 * (r + 1) + 3, "each phase sees all writes");

    int completed = 0;
    std::barrier shrink(2, [&]() noexcept { ++completed; });
    auto leaving = std::async(std::launch::async, [&] { shrink.arrive_and_drop(); });
    shrink.arrive_and_wait();
    leaving.get();
    shrink.arrive_and_wait(); // now one arrival suffices for the next phase
    cs::check(completed == 2, "drop updates current and subsequent expected counts");
}

int main() {
    latch_start(); barrier_phases();
    std::cout << "H1 OK: preparation, gate, phases, completion, split arrival, drop\n";
}
