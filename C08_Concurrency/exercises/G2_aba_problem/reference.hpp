#pragma once
#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <iostream>
#include <thread>

namespace aba_lab {
inline std::uint64_t pack(std::uint32_t index, std::uint32_t version) {
    return (std::uint64_t(version) << 32) | index;
}
// Nodes are indices into a fixed, live array. Signals order ALL next accesses;
// this intentionally wrong logical CAS never performs use-after-free or a race.
inline void run(bool tagged) {
    std::array<std::uint32_t, 3> next{3, 0, 1}; // A=2 -> B=1 -> C=0 -> null=3
    std::atomic<std::uint64_t> head{pack(2, 0)};
    std::atomic<bool> loaded{false}, changed{false};
    auto victim = std::async(std::launch::async, [&] {
        auto old = head.load();
        const auto cached_next = next[std::uint32_t(old)];
        loaded.store(true, std::memory_order_release);
        while (!changed.load(std::memory_order_acquire)) std::this_thread::yield();
        return head.compare_exchange_strong(old, pack(cached_next, tagged ? 1 : 0));
    });
    while (!loaded.load(std::memory_order_acquire)) std::this_thread::yield();
    head.store(pack(1, tagged ? 1 : 0)); // pop A
    head.store(pack(0, tagged ? 2 : 0)); // pop B
    next[2] = 0;                       // reconnect A -> C
    head.store(pack(2, tagged ? 3 : 0)); // push same A
    changed.store(true, std::memory_order_release);
    const bool committed = victim.get(); // transports worker exceptions
    cs::check(committed != tagged, "plain CAS accepts ABA; tagged CAS rejects this history");
    cs::check(std::uint32_t(head.load()) == (tagged ? 2u : 1u), "actual final root");
    cs::check(next[2] == 0, "reinsertion changed A successor");
    std::cout << (tagged ? "tagged" : "plain") << " stale CAS=" << committed
              << " atomic<uint64_t>.is_lock_free=" << head.is_lock_free() << '\n';
}
inline void wrap_model() {
    // A two-bit version aliases after FOUR actual transitions A B A B A.
    auto state = [](unsigned index, unsigned version) { return ((version & 3u) << 2) | index; };
    std::atomic<unsigned> head{state(2, 0)};
    unsigned old = head.load();
    head.store(state(1, 1));
    head.store(state(2, 2));
    head.store(state(1, 3));
    head.store(state(2, 4));
    cs::check(head.compare_exchange_strong(old, state(1, 1)), "tag wrap permits stale CAS again");
}
} // namespace aba_lab
