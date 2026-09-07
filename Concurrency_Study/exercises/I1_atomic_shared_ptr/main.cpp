#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <iostream>
#include <memory>

struct config { unsigned version; unsigned payload; };
int main() {
    using snapshot = std::shared_ptr<const config>;
    std::atomic<snapshot> cell{std::make_shared<const config>(config{1, 2})};
    const snapshot held = cell.load(std::memory_order_acquire);
    cell.store(std::make_shared<const config>(config{2, 4}), std::memory_order_release);
    cs::check(held->version == 1 && held->payload == 2, "reader retains immutable old version");
    // TODO Part 1: weak_ptr lifetime check after dropping last strong owner.
    // TODO Part 2: distinguish independent replacements from cumulative CAS updates.
    // TODO Part 3: concurrent readers check invariant, writers retain all increments.
    std::cout << "Starter: old snapshot survives replacement\n";
}
