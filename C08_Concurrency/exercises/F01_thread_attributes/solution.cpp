#ifndef CS_HAS_STD_THREAD_ATTRIBUTES
#define CS_HAS_STD_THREAD_ATTRIBUTES 0
#endif

#include "concurrency_study/exercise_check.hpp"

#include <iostream>
#include <thread>

#if CS_HAS_STD_THREAD_ATTRIBUTES
#include <atomic>
#endif

int main() try {
#if CS_HAS_STD_THREAD_ATTRIBUTES
    std::thread named(
        std::thread::name_hint<char>{"c08-f01"},
        std::thread::stack_size_hint{0},
        [] {});
    named.join();

    std::atomic<bool> stopped{false};
    std::jthread cancellable(
        std::jthread::name_hint<char>{"c08-f01-jthread"},
        std::jthread::stack_size_hint{64 * 1024},
        [&](std::stop_token st) {
            while (!st.stop_requested()) std::this_thread::yield();
            stopped.store(true);
        });
    cancellable.request_stop();
    cancellable.join();
    cs::check(stopped.load(), "jthread stop token still drives cancellation");
    std::cout << "F01 native C++29 thread attributes OK\n";
    return 0;
#else
    std::cerr << "SKIP: CS_HAS_STD_THREAD_ATTRIBUTES=0; C++29 thread attributes unavailable\n";
    return 77;
#endif
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
