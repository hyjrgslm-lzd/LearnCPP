#include "concurrency_study/exercise_check.hpp"
#include <barrier>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>

struct account { std::mutex mutex; int balance = 10000; };

void transfer(account& from, account& to, int amount, int variant) {
    if (&from == &to) return; // never pass the same non-recursive mutex twice
    auto update = [&] {
        cs::check(amount >= 0 && from.balance >= amount, "valid transfer");
        from.balance -= amount;
        to.balance += amount;
    };
    if (variant == 0) {
        std::scoped_lock lock(from.mutex, to.mutex);
        update();
    } else if (variant == 1) {
        std::lock(from.mutex, to.mutex);
        std::lock_guard first(from.mutex, std::adopt_lock);
        std::lock_guard second(to.mutex, std::adopt_lock);
        update();
    } else {
        const bool from_first = std::less<const void*>{}(&from, &to);
        std::lock_guard first(from_first ? from.mutex : to.mutex);
        std::lock_guard second(from_first ? to.mutex : from.mutex);
        update();
    }
}

#if CS_ENABLE_UNSAFE_DEMOS
void isolated_deadlock() {
    std::mutex a, b;
    std::barrier both_owned(2);
    std::jthread first([&] {
        std::lock_guard own(a);
        both_owned.arrive_and_wait();
        std::lock_guard wait(b);
    });
    std::jthread second([&] {
        std::lock_guard own(b);
        both_owned.arrive_and_wait();
        std::lock_guard wait(a);
    });
}
#endif

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--unsafe-deadlock") {
#if CS_ENABLE_UNSAFE_DEMOS
        isolated_deadlock(); // run ONLY with an external process timeout
        return 0;
#else
        std::cout << "SKIP: unsafe diagnostics disabled\n";
        return 77;
#endif
    }
    cs::check(argc == 1, "unknown argument");
    for (int variant = 0; variant < 3; ++variant) {
        account a, b;
        auto forward = std::async(std::launch::async, [&] {
            for (int i = 0; i < 1000; ++i) transfer(a, b, 1, variant);
        });
        auto reverse = std::async(std::launch::async, [&] {
            for (int i = 0; i < 1000; ++i) transfer(b, a, 2, variant);
        });
        forward.get(); reverse.get();
        transfer(a, a, 1, variant);
        cs::check(a.balance == 11000 && b.balance == 9000, "exact balances, conservation, alias");
    }
    std::cout << "B2 OK: scoped_lock, adopt_lock, total order, self-transfer\n";
}
