#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <iostream>

int main() {
    std::atomic<int> count{0};
    // TODO Part 1: predict this legal interleaving, then add controlled threads.
    const int a = count.load();
    const int b = count.load();
    count.store(a + 1);
    count.store(b + 1);
    cs::check(count.load() == 1, "two split updates can lose one increment");
    // TODO Part 2: check exchange and each fetch operation's OLD return value.
    // TODO Part 3: implement the atomic_flag lock in a separate class (see solution).
    std::atomic_flag flag{};
    cs::check(!flag.test(), "C++20 flag starts clear");
    std::cout << "Starter: split result=1; implement RMW comparison and flag lock\n";
}
