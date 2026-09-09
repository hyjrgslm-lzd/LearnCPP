#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>

int main() {
    int payload = 0;
    std::atomic<bool> ready{false};
    auto producer = std::async(std::launch::async, [&] {
        payload = 42;
        ready.store(true, std::memory_order_release);
        ready.notify_one();
    });
    ready.wait(false, std::memory_order_acquire);
    const int seen = payload; // TODO Part 1: expand payload and write the three HB edges.
    producer.get();
    cs::check(seen == 42, "one-shot publication");
    // TODO Part 2: insert a relaxed CAS relay 1 -> 2, preserving release sequence.
    std::cout << "Starter: publication=42; explain before weakening any memory order\n";
}
