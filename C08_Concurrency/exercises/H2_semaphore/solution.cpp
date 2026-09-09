#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <semaphore>
#include <vector>

void resource_limit() {
    constexpr int limit = 3;
    std::counting_semaphore<limit> permits(limit);
    std::atomic<int> active{0}, peak{0}, completed{0};
    std::vector<std::future<void>> workers;
    workers.reserve(8);
    for (int id = 0; id < 8; ++id)
        workers.push_back(std::async(std::launch::async, [&] {
            for (int i = 0; i < 100; ++i) {
                permits.acquire();
                struct lease {
                    std::counting_semaphore<limit>& permits;
                    std::atomic<int>& active;
                    ~lease() { --active; permits.release(); }
                };
                const int now = ++active;
                lease held{permits, active};
                int previous = peak.load();
                while (now > previous && !peak.compare_exchange_weak(previous, now)) {}
                cs::check(now <= limit, "resource upper bound");
                ++completed;
            }
        }));
    for (auto& worker : workers) worker.get();
    cs::check(active == 0 && completed == 800 && peak >= 1 && peak <= limit,
              "all leases returned; no requirement to hit peak=limit");
    // Verify all permits returned, without assuming try_acquire cannot fail spuriously.
    for (int i = 0; i < limit; ++i) permits.acquire();
    cs::check(!permits.try_acquire(), "zero permits cannot be acquired");
    cs::check(!permits.try_acquire_for(std::chrono::milliseconds(1)), "empty timed acquire fails");
    permits.release(limit);
    cs::check(decltype(permits)::max() >= limit, "template argument is a lower bound on max");
}

void handoff() {
    std::binary_semaphore ready(0), ack(0);
    int payload = 0;
    auto consumer = std::async(std::launch::async, [&] {
        // Do not throw before sending ack: report failures after the handshake.
        bool correct = true;
        for (int i = 1; i <= 100; ++i) {
            ready.acquire();
            correct = correct && payload == i;
            ack.release();
        }
        return correct;
    });
    for (int i = 1; i <= 100; ++i) {
        payload = i;
        ready.release();
        ack.acquire(); // protects the preceding read against the next overwrite
    }
    cs::check(consumer.get(), "two-way handoff publishes non-atomic payload");
}

int main() {
    resource_limit(); handoff();
    std::cout << "H2 OK: upper bound, returned permits, timed failure, 100 handshakes\n";
}
