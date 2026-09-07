#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/queue_baseline.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
    using cs::check;
    using cs::queue_lab::mutex_queue;
    bool rejected = false;
    try { mutex_queue<int> invalid(0); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "zero capacity must be rejected");
    mutex_queue<int> queue(2);
    int value = -1;
    check(!queue.try_pop(value) && value == -1, "empty pop preserves output");
    check(queue.try_push(10) && queue.try_push(20), "two insertions fit");
    check(!queue.try_push(30), "full insertion fails");
    check(queue.try_pop(value) && value == 10, "FIFO first");
    check(queue.try_push(30), "reuse freed capacity");
    check(queue.try_pop(value) && value == 20, "FIFO second");
    check(queue.try_pop(value) && value == 30, "FIFO third");
    check(!queue.try_pop(value), "queue drained");

    mutex_queue<int> single(1);
    check(single.try_push(10), "capacity-one insertion");
    check(!single.try_push(20) && !single.try_push(30), "capacity-one full rejections");
    check(single.try_pop(value) && value == 10, "capacity-one retained value");
    value = -1;
    check(!single.try_pop(value) && value == -1, "capacity-one drained output");

    // Each producer has a disjoint ID range. There is one consumer here so its
    // observation vector needs no lock; this does not yet test MPMC histories.
    constexpr int per_producer = 1000;
    constexpr int producers = 3;
    mutex_queue<int> concurrent(7);
    std::promise<void> release;
    auto start = release.get_future().share();
    std::atomic<bool> cancel{false};
    std::mutex error_mutex;
    std::exception_ptr error;
    std::vector<int> received;
    received.reserve(producers * per_producer);
    std::vector<int> next(producers, 0);
    std::vector<std::jthread> workers;
    workers.reserve(producers);
    try {
        for (int producer = 0; producer < producers; ++producer) {
            workers.emplace_back([&, producer, start] {
                start.wait();
                try {
                    for (int i = 0; i < per_producer; ++i) {
                        const int id = producer * per_producer + i;
                        while (!concurrent.try_push(id)) {
                            if (cancel.load(std::memory_order_relaxed)) return;
                            std::this_thread::yield();
                        }
                    }
                } catch (...) {
                    std::lock_guard lock(error_mutex);
                    if (!error) error = std::current_exception();
                    cancel.store(true, std::memory_order_relaxed);
                }
            });
        }
    } catch (...) {
        cancel.store(true, std::memory_order_relaxed);
        release.set_value();
        throw;
    }
    release.set_value();
    try {
        while (received.size() != producers * per_producer && !cancel.load()) {
            if (concurrent.try_pop(value)) received.push_back(value);
            else std::this_thread::yield();
        }
    } catch (...) {
        cancel.store(true, std::memory_order_relaxed);
        workers.clear();
        throw;
    }
    workers.clear();
    if (error) std::rethrow_exception(error);
    value = -1;
    check(!concurrent.try_pop(value) && value == -1, "no extra items remain after all producers join");
    for (const int id : received) {
        check(id >= 0 && id < producers * per_producer, "ID in range");
        const int producer = id / per_producer;
        check(id % per_producer == next[producer]++, "each producer remains ordered");
    }
    std::sort(received.begin(), received.end());
    for (int id = 0; id < producers * per_producer; ++id)
        check(received[id] == id, "every ID appears exactly once");
    std::cout << "Q0_reference OK: FIFO, capacity, per-producer order, 3000 unique IDs\n";
}
