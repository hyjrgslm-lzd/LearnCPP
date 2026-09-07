#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <thread>
#include <vector>

struct config {
    mutable std::shared_mutex mutex;
    int revision = 0, twice_revision = 0;
    void set(int n) {
        std::lock_guard lock(mutex);
        revision = n;
        twice_revision = 2 * n;
    }
    std::pair<int, int> snapshot() const {
        std::shared_lock lock(mutex);
        return {revision, twice_revision}; // never leak a reference beyond the lock
    }
};

void counter_and_config() {
    int counter = 0;
    std::mutex mutex;
    config state;
    std::vector<std::future<void>> workers;
    for (int id = 0; id < 4; ++id)
        workers.push_back(std::async(std::launch::async, [&] {
            for (int i = 0; i < 2000; ++i) {
                std::lock_guard lock(mutex);
                ++counter;
            }
        }));
    for (auto& worker : workers) worker.get();
    cs::check(counter == 8000, "locked counter exact");
    workers.clear();
    workers.push_back(std::async(std::launch::async, [&] {
        for (int i = 1; i <= 2000; ++i) state.set(i);
    }));
    for (int id = 0; id < 3; ++id)
        workers.push_back(std::async(std::launch::async, [&] {
            for (int i = 0; i < 2000; ++i) {
                auto [version, twice] = state.snapshot();
                cs::check(twice == 2 * version, "shared snapshot invariant");
            }
        }));
    for (auto& worker : workers) worker.get();
    cs::check(state.snapshot() == std::pair{2000, 4000}, "writer final state");
}

void ownership() {
    std::mutex mutex;
    int shared = 42;
    std::unique_lock lock(mutex, std::defer_lock);
    cs::check(!lock.owns_lock(), "deferred lock starts unowned");
    lock.lock();
    const int snapshot = shared;
    std::unique_lock moved(std::move(lock));
    cs::check(!lock.owns_lock() && moved.owns_lock(), "move transfers ownership");
    moved.unlock();
    auto writer = std::async(std::launch::async, [&] {
        std::lock_guard guard(mutex);
        shared = 7;
    });
    writer.get(); // would deadlock if we had kept the lock
    cs::check(snapshot * snapshot == 1764 && shared == 7, "snapshot survives unlock");
    try { std::lock_guard guard(mutex); throw 1; } catch (int) {}
    std::lock_guard reacquired(mutex); // checks exception unwinding released ownership
}

void timed_and_recursive() {
    std::timed_mutex mutex;
    std::unique_lock owner(mutex);
    auto attempt = std::async(std::launch::async, [&] {
        std::unique_lock lock(mutex, std::defer_lock);
        return lock.try_lock_for(std::chrono::milliseconds(1));
    });
    const bool obtained = attempt.get(); // owner cannot unlock before attempt completes
    owner.unlock();
    cs::check(!obtained, "timeout while another thread owns mutex");
    std::lock_guard acquired(mutex); // use blocking lock for guaranteed eventual acquisition
    std::recursive_mutex recursive;
    std::lock_guard outer(recursive);
    { std::lock_guard inner(recursive); }
}

#if CS_ENABLE_UNSAFE_DEMOS
void isolated_race() {
    int counter = 0;
    std::vector<std::jthread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back([&] { for (int j = 0; j < 10000; ++j) ++counter; });
    workers.clear();
    std::cout << "UNDEFINED BEHAVIOR; no expected numeric result: " << counter << '\n';
}
#endif

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--unsafe-race") {
#if CS_ENABLE_UNSAFE_DEMOS
        isolated_race();
        return 0;
#else
        std::cout << "SKIP: unsafe diagnostics disabled\n";
        return 77;
#endif
    }
    cs::check(argc == 1, "unknown argument");
    counter_and_config(); ownership(); timed_and_recursive();
    std::cout << "B1 OK: counter, shared invariant, lock ownership, timed/recursive\n";
}
