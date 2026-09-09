#include "concurrency_study/exercise_check.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <thread>

int main() {
    std::exception_ptr captured;
    std::jthread worker([&] {
        try { throw std::runtime_error("starter worker failure"); }
        catch (...) { captured = std::current_exception(); }
    });
    worker.join();
    cs::check(static_cast<bool>(captured), "worker exception retained");
    try { std::rethrow_exception(captured); }
    catch (const std::runtime_error& e) { std::cout << e.what() << '\n'; }
    // TODO Part 1: add normal and exceptional paths, verify result and error type.
    // TODO Part 2: move a promise into worker; get value/error on main.
    // TODO Part 3: destroy an unsatisfied provider and verify broken_promise.
    // Reading-only error case: removing the worker catch lets the exception
    // escape the thread entry and calls terminate; main's catch cannot catch it.
}
