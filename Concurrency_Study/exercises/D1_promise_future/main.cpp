#include "concurrency_study/exercise_check.hpp"
#include <exception>
#include <future>
#include <iostream>
#include <thread>
#include <utility>

int main() {
    std::promise<int> provider;
    auto result = provider.get_future();
    std::exception_ptr error;
    std::jthread worker([p = std::move(provider), &error]() mutable {
        try { p.set_value(42); }
        catch (...) { error = std::current_exception(); }
    });
    worker.join();
    if (error) std::rethrow_exception(error);
    cs::check(result.get() == 42 && !result.valid(), "one result consumed");
    std::cout << "worker joined, get=42, valid=0\n";
    // TODO Part 1: wait then read an ordinary value published before fulfillment.
    // TODO Part 2: transport a real calculation exception via set_exception.
    // TODO Part 3: share a result to three consumers; join before checking outputs.
    // TODO Part 4: early-return abandonment and a promise<void> completion.
}
