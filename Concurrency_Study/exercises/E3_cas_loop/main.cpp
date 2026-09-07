#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <iostream>

int main() {
    std::atomic<int> value{5};
    int expected = 4;
    cs::check(!value.compare_exchange_strong(expected, 9), "mismatching CAS");
    cs::check(expected == 5 && value.load() == 5, "failure writes expected");
    // TODO Part 1: add raise_max and the always-RMW variant.
    // TODO Part 2: add unsigned checked multiplication, recomputing after failures.
    // TODO Part 3: put native fetch_min/max behind CS_HAS_ATOMIC_MIN_MAX.
    std::cout << "Starter: failed CAS refilled expected=5\n";
}
