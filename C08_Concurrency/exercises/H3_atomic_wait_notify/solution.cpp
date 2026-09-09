#include "concurrency_study/exercise_check.hpp"

#include <atomic>
#include <future>
#include <iostream>
#include <thread>

void one_shot(bool use_wait) {
    int payload = 0;
    std::atomic<bool> ready{false};
    auto reader = std::async(std::launch::async, [&] {
        if (use_wait) ready.wait(false, std::memory_order_acquire);
        else while (!ready.load(std::memory_order_acquire)) std::this_thread::yield();
        cs::check(payload == 42, "one-shot ordinary payload published");
    });
    payload = 42;
    ready.store(true, std::memory_order_release);
    ready.notify_one();
    reader.get();
}

void generation_history() {
    std::atomic<bool> bit{false}, armed{false}, inspect{false};
    std::atomic<unsigned> generation{0};
    auto reader = std::async(std::launch::async, [&] {
        const bool old_bit = bit.load(std::memory_order_relaxed);
        const unsigned old_generation = generation.load(std::memory_order_relaxed);
        armed.store(true, std::memory_order_release);
        armed.notify_one();
        inspect.wait(false, std::memory_order_acquire);
        // The gate intentionally places BOTH events before this observation.
        const bool same_bit = bit.load(std::memory_order_relaxed) == old_bit;
        generation.wait(old_generation, std::memory_order_acquire);
        const unsigned delta = generation.load(std::memory_order_relaxed) - old_generation;
        cs::check(same_bit && delta == 2, "bool loses history; generation retains two events");
    });
    armed.wait(false, std::memory_order_acquire);
    bit.store(true, std::memory_order_relaxed);
    generation.fetch_add(1, std::memory_order_release);
    bit.store(false, std::memory_order_relaxed);
    generation.fetch_add(1, std::memory_order_release);
    generation.notify_one();
    inspect.store(true, std::memory_order_release);
    inspect.notify_one();
    reader.get();
}

// No event is missed even when several increments are coalesced into one wake.
// This counts events; it does not expose an overwriteable non-atomic payload.
void generation_stream() {
    constexpr unsigned events = 1000;
    std::atomic<unsigned> generation{0};
    auto reader = std::async(std::launch::async, [&] {
        unsigned seen = 0, accounted = 0;
        while (seen != events) {
            generation.wait(seen, std::memory_order_relaxed);
            const unsigned current = generation.load(std::memory_order_relaxed);
            accounted += current - seen;
            seen = current;
        }
        cs::check(accounted == events, "coalesced notifications preserve event count");
    });
    for (unsigned i = 0; i < events; ++i) {
        generation.fetch_add(1, std::memory_order_relaxed);
        generation.notify_one();
    }
    reader.get();
}

int main() {
    one_shot(true);
    one_shot(false); // same correctness contract; no speed or CPU-usage assertion
    generation_history();
    generation_stream();
    std::cout << "H3_reference OK: one-shot, controlled ABA history, 1000 generations\n";
}
