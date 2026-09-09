#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

// Part 1: force a legal lost update without a data race or timing assumption.
void split_increment() {
    std::atomic<int> count{0};
    std::promise<void> loaded, resume;
    auto go = resume.get_future();
    auto worker = std::async(std::launch::async, [&] {
        const int old = count.load();
        loaded.set_value();
        go.wait();
        count.store(old + 1);
    });
    loaded.get_future().wait();
    const int old = count.load();
    count.store(old + 1);
    resume.set_value();
    worker.get();
    cs::check(count.load() == 1, "two split increments overwrite each other");

    count.store(0);
    std::array<std::future<void>, 4> workers;
    for (auto& task : workers)
        task = std::async(std::launch::async, [&] {
            for (int i = 0; i < 2000; ++i) count.fetch_add(1);
        });
    for (auto& task : workers) task.get();
    cs::check(count.load() == 8000, "RMW counts every increment");
}

void operations() {
    std::atomic<int> value{10};
    cs::check(value.load() == 10, "explicit initial value");
    value.store(42);
    cs::check(value.exchange(7) == 42, "exchange returns old value");
    cs::check(value.fetch_add(3) == 7, "fetch_add returns old value");
    cs::check(value.fetch_sub(2) == 10 && value.load() == 8, "fetch_sub");
    int implicit = value; // operator T() exists; explicit load makes order visible.
    cs::check(implicit == 8 && ++value == 9 && value++ == 9, "conversion and increment");
    std::atomic<unsigned> flags{1};
    cs::check(flags.fetch_or(4) == 1 && flags.load() == 5, "or");
    cs::check(flags.fetch_and(6) == 5 && flags.load() == 4, "and");
    cs::check(flags.fetch_xor(2) == 4 && flags.load() == 6, "xor");
    std::cout << "atomic<int>: runtime lock-free=" << value.is_lock_free()
              << ", always=" << decltype(value)::is_always_lock_free << '\n';
}

class spin_mutex {
    std::atomic_flag held_{}; // C++20 default construction initializes clear.
public:
    void lock() noexcept {
        // ponytail: teaching spin loop has no fairness; use std::mutex for blocking work.
        while (held_.test_and_set(std::memory_order_acquire))
            std::this_thread::yield();
    }
    void unlock() noexcept { held_.clear(std::memory_order_release); }
};

void flag_lock() {
    std::atomic_flag probe{};
    cs::check(!probe.test(), "default flag is clear");
    cs::check(!probe.test_and_set() && probe.test_and_set(), "test-and-set old state");
    probe.clear(std::memory_order_release);
    cs::check(!probe.test(), "clear is a store");

    spin_mutex mutex;
    int guarded = 0;
    std::array<std::future<void>, 4> workers;
    for (auto& task : workers)
        task = std::async(std::launch::async, [&] {
            for (int i = 0; i < 2000; ++i) {
                std::lock_guard lock(mutex);
                ++guarded;
            }
        });
    for (auto& task : workers) task.get();
    cs::check(guarded == 8000, "flag protocol protects ordinary memory");
}

int main() {
    split_increment();
    operations();
    flag_lock();
    std::cout << "E1_reference OK: split/RMW, operations, flag lock\n";
}
