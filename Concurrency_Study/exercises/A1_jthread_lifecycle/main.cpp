#include "concurrency_study/exercise_check.hpp"
#include <iostream>
#include <thread>

int main() {
    int result = 0;
    std::thread manual([&] { result = 42; });
    manual.join(); // Keep cleanup while experimenting.
    cs::check(result == 42 && !manual.joinable(), "manual join baseline");
    std::cout << "thread joined, result=42\n";
    {
        std::jthread automatic([&] { result = 7; });
        // TODO Part 2: move automatic into a new owner and check joinable().
    }
    cs::check(result == 7, "scope exit joins");
    std::cout << "jthread scope ended, result=7\n";
    // TODO Part 1: catch the system_error from a SECOND explicit join.
    // TODO Part 3: add a stop-token worker; let destructor request stop.
    // TODO Part 4: compare value, std::ref, and move-only capture.
    // Reading-only error case: destroying a joinable std::thread terminates.
    // Do not remove manual.join() in this executable.
}
