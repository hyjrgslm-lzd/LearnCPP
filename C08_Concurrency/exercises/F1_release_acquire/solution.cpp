#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <thread>

struct payload { int id = 0; std::array<int, 3> values{}; };

void direct_publication() {
    payload data;
    std::atomic<bool> ready{false};
    auto producer = std::async(std::launch::async, [&] {
        data = {7, {11, 22, 33}};
        ready.store(true, std::memory_order_release);
        ready.notify_one();
    });
    ready.wait(false, std::memory_order_acquire);
    const payload observed = data; // before get(): ready is the publication edge
    producer.get();
    cs::check(observed.id == 7 && observed.values == std::array{11, 22, 33},
              "acquire observes complete ordinary payload");
}

void release_sequence() {
    payload data;
    std::atomic<int> phase{0};
    auto producer = std::async(std::launch::async, [&] {
        data = {9, {4, 5, 6}};
        phase.store(1, std::memory_order_release); // release-sequence head
    });
    auto relay = std::async(std::launch::async, [&] {
        int expected = 1;
        while (!phase.compare_exchange_weak(expected, 2,
                                             std::memory_order_relaxed,
                                             std::memory_order_relaxed)) {
            expected = 1; // must transition 1 -> 2, never accidentally 0 -> 2
            std::this_thread::yield();
        }
        phase.notify_one();
    });
    int seen = phase.load(std::memory_order_acquire);
    while (seen != 2) {
        phase.wait(seen, std::memory_order_acquire);
        seen = phase.load(std::memory_order_acquire);
    }
    const payload observed = data; // reads through relaxed RMW release sequence
    producer.get();
    relay.get();
    cs::check(observed.id == 9 && observed.values == std::array{4, 5, 6},
              "relaxed RMW extends release sequence");
}

int main() {
    direct_publication();
    release_sequence();
    std::cout << "F1_reference OK: direct publication and RMW relay\n";
}
