#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct calculation_failure : std::runtime_error { using std::runtime_error::runtime_error; };

void part1_and_2_transfer(bool fail) {
    std::promise<int> provider;
    auto result = provider.get_future();
    std::exception_ptr transport_error;
    int published_input = 0;
    std::jthread worker([p = std::move(provider), &transport_error, &published_input, fail]() mutable {
        try {
            try {
                published_input = 21;
                if (fail) throw calculation_failure("cannot calculate");
                p.set_value(published_input * 2);
            } catch (...) { p.set_exception(std::current_exception()); }
        } catch (...) { transport_error = std::current_exception(); }
    });
    // Wait/get may finish before the worker exits. Published input is only
    // written before fulfillment, so a successful wait suffices to read it.
    result.wait();
    const int observed = published_input;
    worker.join();
    if (transport_error) std::rethrow_exception(transport_error);
    cs::check(observed == 21, "fulfillment publishes preceding write");
    bool caught = false;
    int value = 0;
    try { value = result.get(); }
    catch (const calculation_failure& e) { caught = std::string(e.what()) == "cannot calculate"; }
    cs::check(caught == fail && (fail || value == 42), "value or original exception");
    cs::check(!result.valid(), "get consumes association");
    std::cout << "Part " << (fail ? 2 : 1) << ": published input=21; "
              << (fail ? "get rethrows calculation_failure" : "get=42") << '\n';
}

void part3_broadcast() {
    std::array<int, 3> values{};
    std::array<std::exception_ptr, 3> errors{};
    std::vector<std::jthread> consumers;
    consumers.reserve(values.size());
    // Declared after consumers: unwinding destroys provider before joining
    // waiting consumers, publishing broken_promise if construction fails.
    std::promise<int> provider;
    auto unique = provider.get_future();
    auto shared = unique.share();
    cs::check(!unique.valid(), "share consumes unique future");
    for (std::size_t i = 0; i < values.size(); ++i) {
        consumers.emplace_back([shared, i, &values, &errors] {
            try {
                values[i] = shared.get();
                cs::check(shared.get() == values[i], "each consumer can get repeatedly");
            } catch (...) { errors[i] = std::current_exception(); }
        });
    }
    provider.set_value(100);
    consumers.clear(); // Join all before examining either array.
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (errors[i]) std::rethrow_exception(errors[i]);
        cs::check(values[i] == 100, "every consumer receives the same result");
    }
    cs::check(shared.valid() && shared.get() == 100, "shared association remains valid");
    std::cout << "Part 3: consumers[0..2]=100; repeated shared get succeeds\n";
}

void part4_abandon_and_event() {
    std::promise<int> provider;
    auto result = provider.get_future();
    {
        // Owning parameter is destroyed when this invocation returns.
        std::jthread worker([](std::promise<int>) {}, std::move(provider));
    }
    bool broken = false;
    try { (void)result.get(); }
    catch (const std::future_error& e) { broken = e.code() == std::future_errc::broken_promise; }
    cs::check(broken, "normal early return abandons unsatisfied provider");
    std::promise<void> provider_done;
    auto done = provider_done.get_future();
    std::exception_ptr error;
    std::jthread worker([p = std::move(provider_done), &error]() mutable {
        try { p.set_value(); }
        catch (...) { error = std::current_exception(); }
    });
    worker.join();
    if (error) std::rethrow_exception(error);
    done.get();
    cs::check(!done.valid(), "void completion consumed");
    std::cout << "Part 4: early return -> broken_promise; void event consumed\n";
}

int main() {
    part1_and_2_transfer(false);
    part1_and_2_transfer(true);
    part3_broadcast();
    part4_abandon_and_event();
    std::cout << "D1_reference OK\n";
}
