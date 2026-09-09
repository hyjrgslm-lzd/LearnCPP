#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>

int main() {
    std::atomic<int> count{0};
    auto worker = std::async(std::launch::async, [&] {
        for (int i = 0; i < 1000; ++i) count.fetch_add(1, std::memory_order_relaxed);
    });
    for (int i = 0; i < 1000; ++i) count.fetch_add(1, std::memory_order_relaxed);
    worker.get();
    cs::check(count.load(std::memory_order_relaxed) == 2000, "count after completion");
    // TODO Part 1: expand to four workers and independently check their local totals.
    // TODO Part 2: add a finite atomic-only message-passing litmus; never demand stale > 0.
    std::cout << "Starter: relaxed count=2000\n";
}
