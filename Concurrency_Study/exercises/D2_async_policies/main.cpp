#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

int main() {
    const auto caller = std::this_thread::get_id();
    auto result = std::async(std::launch::deferred, [] { return std::this_thread::get_id(); });
    cs::check(result.wait_for(std::chrono::seconds(0)) == std::future_status::deferred,
              "deferred before first non-timed wait");
    cs::check(result.get() == caller, "deferred executes on get caller");
    std::cout << "deferred -> get on calling thread\n";
    // TODO Part 1: explicit async handshake and default-policy observation.
    // TODO Part 2: compare temporary release and retained futures using a gate.
    // TODO Part 3: copy shared_future and release first/last references separately.
    // TODO Part 4: test get rethrowing under both explicit policies.
    // No sleep and no expected speedup: prove allowed ordering with synchronization.
}
