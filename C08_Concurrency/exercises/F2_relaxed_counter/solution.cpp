#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <barrier>
#include <future>
#include <iostream>

void count_after_join() {
    std::atomic<int> total{0};
    std::array<int, 4> local{};
    std::array<std::future<void>, 4> workers;
    for (int t = 0; t < 4; ++t)
        workers[t] = std::async(std::launch::async, [&, t] {
            for (int i = 0; i < 2000; ++i) {
                ++local[t]; // disjoint ordinary elements; read only after get()
                total.fetch_add(1, std::memory_order_relaxed);
            }
        });
    for (auto& task : workers) task.get();
    cs::check(total.load(std::memory_order_relaxed) == 8000, "relaxed total after join");
    for (int n : local) cs::check(n == 2000, "future completion publishes local results");
}

// Finite rounds; barriers bound each window, never sit between flag and data.
// Both payload and flag are atomic: stale observations are defined, not UB.
int message_passing(bool synchronized) {
    constexpr int rounds = 4000;
    std::atomic<int> data{0}, flag{0};
    std::barrier phase(2);
    int stale = 0;
    auto producer = std::async(std::launch::async, [&] {
        for (int i = 0; i < rounds; ++i) {
            phase.arrive_and_wait();
            data.store(1, std::memory_order_relaxed);
            flag.store(1, synchronized ? std::memory_order_release : std::memory_order_relaxed);
            phase.arrive_and_wait();
        }
    });
    for (int i = 0; i < rounds; ++i) {
        data.store(0, std::memory_order_relaxed);
        flag.store(0, std::memory_order_relaxed);
        phase.arrive_and_wait();
        const int f = flag.load(synchronized ? std::memory_order_acquire : std::memory_order_relaxed);
        const int d = data.load(std::memory_order_relaxed);
        stale += f == 1 && d == 0;
        phase.arrive_and_wait();
    }
    producer.get();
    return stale;
}

int main() {
    count_after_join();
    const int relaxed_stale = message_passing(false);
    const int synchronized_stale = message_passing(true);
    cs::check(synchronized_stale == 0, "RA forbids flag=1,data=0");
    // relaxed_stale == 0 is permitted and is NOT evidence for publication.
    std::cout << "F2_reference OK: total=8000, relaxed stale=" << relaxed_stale
              << ", RA stale=" << synchronized_stale << '\n';
}
