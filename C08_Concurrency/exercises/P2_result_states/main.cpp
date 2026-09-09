#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <future>
#include <iostream>

int main() {
    std::promise<int> provider;
    auto result = provider.get_future();
    cs::check(result.valid(), "valid before fulfillment");
    cs::check(result.wait_for(std::chrono::seconds(0)) == std::future_status::timeout,
              "no result yet");
    std::cout << "valid=1, ready=0\n";
    provider.set_value(42);
    result.wait();
    cs::check(result.get() == 42 && !result.valid(), "get consumes future");
    std::cout << "get=42, valid=0\n";
    // TODO Part 1: also check a default future and move the valid future.
    // TODO Part 2: send an exception; distinguish wait from get.
    // TODO Part 3: check duplicate fulfillment and provider abandonment.
    // TODO Part 4: share a result, read twice, and add promise<void>.
}
