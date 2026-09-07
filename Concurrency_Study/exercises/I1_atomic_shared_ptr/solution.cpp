#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <memory>

struct config { unsigned version = 0; unsigned payload = 0; };
using snapshot = std::shared_ptr<const config>;

// Replacing a snapshot means last-store-wins. Updating from the old value needs CAS.
void increment(std::atomic<snapshot>& cell) {
    snapshot old = cell.load(std::memory_order_acquire);
    for (;;) {
        // Objects are const from construction: no retained mutable alias.
        snapshot next = std::make_shared<const config>(
            config{old->version + 1, (old->version + 1) * 2});
        if (cell.compare_exchange_weak(old, next,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) return;
        // Failure refills old with an owning, acquired snapshot; rebuild next.
    }
}

int main() {
    std::atomic<snapshot> cell{std::make_shared<const config>(config{0, 0})};
    snapshot held = cell.load(std::memory_order_acquire);
    std::weak_ptr<const config> old_lifetime = held;
    cell.store(std::make_shared<const config>(config{10, 20}), std::memory_order_release);
    cs::check(!old_lifetime.expired() && held->version == 0 && held->payload == 0,
              "reader owns old immutable snapshot after replacement");
    held.reset();
    cs::check(old_lifetime.expired(), "old snapshot released when last owner leaves");

    // Controlled numeric lost update: two independent stores derived from the same base.
    const snapshot a = cell.load(std::memory_order_acquire);
    const snapshot b = cell.load(std::memory_order_acquire);
    cell.store(std::make_shared<const config>(config{a->version + 1, (a->version + 1) * 2}),
               std::memory_order_release);
    cell.store(std::make_shared<const config>(config{b->version + 1, (b->version + 1) * 2}),
               std::memory_order_release);
    cs::check(cell.load()->version == 11, "store is safe replacement, not cumulative update");

    cell.store(std::make_shared<const config>(config{0, 0}), std::memory_order_release);
    std::array<std::future<void>, 4> tasks;
    for (int t = 0; t < 2; ++t)
        tasks[t] = std::async(std::launch::async, [&] {
            for (int i = 0; i < 1000; ++i) increment(cell);
        });
    for (int t = 2; t < 4; ++t)
        tasks[t] = std::async(std::launch::async, [&] {
            for (int i = 0; i < 3000; ++i) {
                const snapshot copy = cell.load(std::memory_order_acquire);
                cs::check(copy && copy->payload == copy->version * 2, "immutable invariant");
            }
        });
    for (auto& task : tasks) task.get(); // allocation/check exceptions reach main
    const snapshot final = cell.load(std::memory_order_acquire);
    cs::check(final->version == 2000 && final->payload == 4000, "CAS retains every update");
    std::cout << "I1_reference OK: lifetime, replacement, 2000 cumulative updates; lock-free="
              << cell.is_lock_free() << '\n';
}
