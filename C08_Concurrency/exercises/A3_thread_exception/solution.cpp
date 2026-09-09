#include "concurrency_study/exercise_check.hpp"
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

struct job_failure : std::runtime_error { using std::runtime_error::runtime_error; };

void part1_manual(bool fail) {
    std::exception_ptr captured;
    int value = 0;
    std::jthread worker([&] {
        try {
            if (fail) throw job_failure("manual failure");
            value = 42;
            cs::check(value == 42, "normal worker path");
        } catch (...) { captured = std::current_exception(); }
    });
    worker.join(); // The first read of captured and value occurs AFTER this.
    bool caught = false;
    try { if (captured) std::rethrow_exception(captured); }
    catch (const job_failure& e) { caught = std::string(e.what()) == "manual failure"; }
    cs::check(caught == fail && (fail || value == 42), "manual value/error paths");
    std::cout << "Part 1: joined -> " << (caught ? "original failure rethrown" : "value 42") << '\n';
}

void part2_promise(bool fail) {
    std::promise<int> provider;
    auto result = provider.get_future();
    std::exception_ptr transport_error;
    std::jthread worker([p = std::move(provider), &transport_error, fail]() mutable {
        try {
            try {
                if (fail) throw job_failure("promise failure");
                p.set_value(42);
            } catch (...) { p.set_exception(std::current_exception()); }
        } catch (...) { transport_error = std::current_exception(); }
    });
    worker.join();
    if (transport_error) std::rethrow_exception(transport_error);
    bool caught = false;
    int value = 0;
    try { value = result.get(); }
    catch (const job_failure& e) { caught = std::string(e.what()) == "promise failure"; }
    cs::check(caught == fail && (fail || value == 42), "promise value/error paths");
    cs::check(!result.valid(), "get invalidates on either path");
    std::cout << "Part 2: " << (caught ? "get rethrows failure" : "get returns 42") << '\n';
}

void part3_abandoned() {
    std::future<int> result;
    {
        std::promise<int> provider;
        result = provider.get_future();
    }
    bool caught = false;
    try { (void)result.get(); }
    catch (const std::future_error& e) { caught = e.code() == std::future_errc::broken_promise; }
    cs::check(caught, "abandonment is distinct from job_failure");
    cs::check(!std::current_exception(), "outside active handler there is no current exception");
    std::cout << "Part 3: abandoned provider -> broken_promise\n";
}

int main() {
    part1_manual(false);
    part1_manual(true);
    part2_promise(false);
    part2_promise(true);
    part3_abandoned();
    std::cout << "A3_reference OK\n";
}
