#ifndef CS_HAS_STD_SENDERS
#define CS_HAS_STD_SENDERS 0
#endif

#include "concurrency_study/exercise_check.hpp"

#include <iostream>

#if CS_HAS_STD_SENDERS
#include <execution>
#include <tuple>
#endif

int main() try {
#if CS_HAS_STD_SENDERS
    auto result = std::this_thread::sync_wait(
        std::execution::just(21) | std::execution::then([](int value) { return value * 2; }));
    cs::check(result && std::get<0>(*result) == 42, "native standard sender value pipeline");
    std::cout << "F03 native std sender OK\n";
    return 0;
#else
    std::cerr << "SKIP: CS_HAS_STD_SENDERS=0; native standard sender unavailable\n";
    return 77;
#endif
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
