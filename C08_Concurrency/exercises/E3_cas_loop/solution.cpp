#include "concurrency_study/exercise_check.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>

// Numeric maximum only: the shortcut may return after a LOAD, without any RMW.
int raise_max(std::atomic<int>& target, int candidate) {
    int old = target.load(std::memory_order_relaxed);
    while (old < candidate &&
           !target.compare_exchange_weak(old, candidate,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed)) {}
    return old;
}

// Always completes a successful RMW, including when the numeric value is unchanged.
int fetch_max_rmw(std::atomic<int>& target, int candidate) {
    int old = target.load(std::memory_order_relaxed);
    while (!target.compare_exchange_weak(old, std::max(old, candidate),
                                        std::memory_order_relaxed,
                                        std::memory_order_relaxed)) {}
    return old;
}

// Restrict to unsigned multiplication; reject overflow instead of silently wrapping.
unsigned fetch_multiply(std::atomic<unsigned>& target, unsigned factor) {
    unsigned old = target.load(std::memory_order_relaxed);
    for (;;) {
        if (factor != 0 && old > std::numeric_limits<unsigned>::max() / factor)
            throw std::overflow_error("product out of range");
        const unsigned desired = old * factor; // recompute after expected is refilled
        if (target.compare_exchange_weak(old, desired,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed))
            return old;
    }
}

int main() {
    std::atomic<int> value{5};
    int expected = 4;
    cs::check(!value.compare_exchange_strong(expected, 9,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire),
              "mismatch fails");
    cs::check(expected == 5 && value.load() == 5, "failure refills expected only");
    cs::check(value.compare_exchange_strong(expected, 9), "matching strong succeeds");
    cs::check(expected == 5 && value.load() == 9, "success leaves expected unchanged");
    cs::check(raise_max(value, 8) == 9 && value.load() == 9, "shortcut maximum");
    cs::check(fetch_max_rmw(value, 8) == 9 && value.load() == 9, "unchanged-value RMW");

    std::atomic<int> hi{-1};
    std::array<std::future<void>, 4> workers;
    for (int t = 0; t < 4; ++t)
        workers[t] = std::async(std::launch::async, [&, t] {
            for (int i = 0; i < 1000; ++i) raise_max(hi, t * 1000 + i);
        });
    for (auto& task : workers) task.get();
    cs::check(hi.load() == 3999, "concurrent maximum");

    std::atomic<unsigned> product{1};
    for (int t = 0; t < 4; ++t)
        workers[t] = std::async(std::launch::async, [&] {
            for (int i = 0; i < 4; ++i) fetch_multiply(product, 2);
        });
    for (auto& task : workers) task.get();
    cs::check(product.load() == 65536, "concurrent multiply recomputes desired");
    product.store(std::numeric_limits<unsigned>::max());
    bool rejected = false;
    try { fetch_multiply(product, 2); }
    catch (const std::overflow_error&) { rejected = true; }
    cs::check(rejected && product.load() == std::numeric_limits<unsigned>::max(),
              "overflow leaves target unchanged");
    cs::check(fetch_multiply(product, 0) == std::numeric_limits<unsigned>::max() &&
              product.load() == 0, "multiply by zero");

#if CS_HAS_ATOMIC_MIN_MAX
    std::atomic<int> native{7};
    cs::check(native.fetch_max(3, std::memory_order_relaxed) == 7, "native unchanged RMW");
    cs::check(native.fetch_max(9) == 7 && native.load() == 9, "native maximum");
    cs::check(native.fetch_min(4) == 9 && native.load() == 4, "native minimum");
#else
    std::cout << "SKIP native C++26 fetch_min/max: lane disabled or probe not passed; "
                 "see CONCURRENCY_STUDY_ENABLE_CXX26 and capabilities log\n";
#endif
    std::cout << "E3_reference OK: expected, max, checked multiply\n";
}
