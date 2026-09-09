#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <string_view>
#include <vector>

int main(int argc, char** argv) {
    std::mutex mutex;
    std::condition_variable cv, arrived;
    bool ready = false;
    int payload = 0, waiting = 0, predicate_checks = 0;
    if (argc == 2 && std::string_view(argv[1]) == "--unsafe-notify") {
#if CS_ENABLE_UNSAFE_DEMOS
        ready = true;
        cv.notify_one(); // no waiter exists yet; this notification is not stored
        std::unique_lock lock(mutex);
        auto status = cv.wait_for(lock, std::chrono::milliseconds(5));
        std::cout << "bare wait status=" << (status == std::cv_status::timeout)
                  << "; ready=" << ready << "; spurious wake is also legal\n";
        return 0;
#else
        std::cout << "SKIP: unsafe diagnostics disabled\n";
        return 77;
#endif
    }
    cs::check(argc == 1, "unknown argument");
    // Part 1a: notification before wait. Stored state makes this safe.
    ready = true; payload = 42;
    cv.notify_one();
    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return ready; });
        cs::check(payload == 42, "early publication remains observable");
    }
    ready = false;
    std::vector<std::future<int>> workers;
    workers.reserve(3);
    try {
        for (int i = 0; i < 3; ++i)
            workers.push_back(std::async(std::launch::async, [&] {
                std::unique_lock lock(mutex);
                ++waiting;
                arrived.notify_one();
                cv.wait(lock, [&] {
                    ++predicate_checks;
                    arrived.notify_one();
                    return ready;
                });
                return payload;
            }));
    } catch (...) {
        { std::lock_guard lock(mutex); ready = true; }
        cv.notify_all();
        throw;
    }
    {
        std::unique_lock lock(mutex);
        arrived.wait(lock, [&] { return waiting == 3; });
        // Same mutex: all three have released it through wait at least once.
        cv.notify_all(); // deliberate unrelated notification; ready is still false
        arrived.wait(lock, [&] { return predicate_checks >= 6; });
        payload = 99;
        ready = true;
    }
    cv.notify_all();
    for (auto& worker : workers) cs::check(worker.get() == 99, "broadcast publishes payload");

    std::unique_lock lock(mutex);
    ready = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
    cs::check(!cv.wait_until(lock, deadline, [&] { return ready; }), "false predicate times out");
    ready = true;
    cs::check(cv.wait_until(lock, deadline, [&] { return ready; }),
              "predicate checked even when deadline has passed");
    std::cout << "C1 OK: early state, broadcast, unrelated notify, deadline predicates\n";
}
