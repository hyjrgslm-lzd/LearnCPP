#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>

int main() {
    int slot = 0;
    std::atomic<bool> ready{false};
    auto consumer = std::async(std::launch::async, [&] {
        ready.wait(false, std::memory_order_acquire);
        cs::check(slot == 7, "single publication");
    });
    slot = 7;
    ready.store(true, std::memory_order_release);
    ready.notify_one();
    consumer.get();
    // TODO: before reusing slot while consumer lives, add its RELEASE acknowledgement
    // and the producer's matching ACQUIRE. Simply incrementing a version is insufficient.
    std::cout << "Starter: one handoff complete; extend to reusable SPSC slot\n";
}
