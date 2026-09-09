#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <future>
#include <iostream>

struct payload { int version = 0; std::array<int, 3> values{}; };

// SPSC, capacity 1, exactly-once delivery. Reader acknowledgement permits reuse.
int main() {
    constexpr int rounds = 2000;
    payload slot;
    std::atomic<int> published{0}, acknowledged{0};
    auto consumer = std::async(std::launch::async, [&] {
        bool valid = true;
        for (int version = 1; version <= rounds; ++version) {
            int seen = published.load(std::memory_order_acquire);
            while (seen < version) {
                published.wait(seen, std::memory_order_acquire);
                seen = published.load(std::memory_order_acquire);
            }
            const payload copy = slot;
            valid &= seen == version && copy.version == version &&
                     copy.values == std::array{version, version * 2, -version};
            acknowledged.store(version, std::memory_order_release);
            acknowledged.notify_one();
        }
        // Delay throwing until every acknowledgement is sent: no stranded producer.
        cs::check(valid, "every delivered version is exact and internally consistent");
    });
    for (int version = 1; version <= rounds; ++version) {
        int seen = acknowledged.load(std::memory_order_acquire);
        while (seen != version - 1) {
            acknowledged.wait(seen, std::memory_order_acquire);
            seen = acknowledged.load(std::memory_order_acquire);
        }
        slot = {version, {version, version * 2, -version}};
        published.store(version, std::memory_order_release);
        published.notify_one();
    }
    consumer.get(); // propagates worker check failures; all accesses have ended
    cs::check(acknowledged.load() == rounds, "final acknowledgement before destruction");
    std::cout << "F4_reference OK: 2000 exact SPSC handoffs and safe slot reuse\n";
}
