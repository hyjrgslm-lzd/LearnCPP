#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>

int main() {
    int payload = 0;
    std::atomic<unsigned> generation{0};
    auto reader = std::async(std::launch::async, [&] {
        generation.wait(0, std::memory_order_acquire);
        cs::check(payload == 7, "acquire wait publishes one-shot payload");
    });
    payload = 7;
    generation.store(1, std::memory_order_release);
    generation.notify_one();
    reader.get();
    // TODO Part 1: contrast finite one-shot wait/spin without claiming a speedup.
    // TODO Part 2: force two changes before observation; compare bool with generation.
    // TODO Part 3: count generation DELTAS, not notify calls; do not overwrite payload.
    std::cout << "Starter: one-shot generation=1\n";
}
