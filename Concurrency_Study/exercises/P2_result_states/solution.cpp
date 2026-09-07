#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using namespace std::chrono_literals;

void part1_states() {
    std::future<int> empty;
    cs::check(!empty.valid(), "default future has no shared state");
    std::promise<int> provider;
    auto first = provider.get_future();
    cs::check(first.valid() && first.wait_for(0s) == std::future_status::timeout,
              "valid does not mean ready");
    auto result = std::move(first);
    cs::check(!first.valid() && result.valid(), "move transfers state association");
    provider.set_value(42);
    cs::check(result.wait_for(0s) == std::future_status::ready, "fulfilled state ready");
    result.wait();
    result.wait();
    cs::check(result.valid(), "wait does not consume");
    cs::check(result.get() == 42 && !result.valid(), "get consumes");
    std::cout << "Part 1: empty -> valid/pending -> moved -> ready -> get 42 -> invalid\n";
}

void part2_exception() {
    std::promise<int> provider;
    auto result = provider.get_future();
    try { throw std::runtime_error("calculation failed"); }
    catch (...) { provider.set_exception(std::current_exception()); }
    cs::check(result.wait_for(0s) == std::future_status::ready, "exception is ready");
    result.wait(); // Does not rethrow the stored exception.
    bool caught = false;
    try { (void)result.get(); }
    catch (const std::runtime_error& e) { caught = std::string(e.what()) == "calculation failed"; }
    cs::check(caught && !result.valid(), "exceptional get consumes too");
    cs::check(!std::current_exception(), "no exception currently handled here");
    std::cout << "Part 2: ready(exception), wait returns, get rethrows, invalid\n";
}

void part3_provider_errors() {
    std::promise<int> provider;
    auto result = provider.get_future();
    bool duplicate_future = false;
    try { (void)provider.get_future(); }
    catch (const std::future_error& e) {
        duplicate_future = e.code() == std::future_errc::future_already_retrieved;
    }
    provider.set_value(1);
    bool duplicate_value = false;
    try { provider.set_value(2); }
    catch (const std::future_error& e) {
        duplicate_value = e.code() == std::future_errc::promise_already_satisfied;
    }
    cs::check(duplicate_future && duplicate_value && result.get() == 1, "one-shot contract");
    std::future<int> abandoned;
    {
        std::promise<int> unfinished;
        abandoned = unfinished.get_future();
    }
    cs::check(abandoned.wait_for(0s) == std::future_status::ready, "abandonment makes ready");
    bool broken = false;
    try { (void)abandoned.get(); }
    catch (const std::future_error& e) { broken = e.code() == std::future_errc::broken_promise; }
    cs::check(broken, "broken_promise is an exceptional result");
    std::cout << "Part 3: duplicate future/value rejected; abandonment -> broken_promise\n";
}

void part4_shared_and_void() {
    std::promise<std::string> provider;
    auto unique = provider.get_future();
    auto shared = unique.share();
    auto other = shared;
    cs::check(!unique.valid() && shared.valid() && other.valid(), "share transfers association");
    provider.set_value("configuration");
    static_assert(std::is_same_v<decltype(shared.get()), const std::string&>);
    cs::check(shared.get() == "configuration" && &shared.get() == &other.get(), "shared result");
    cs::check(shared.valid() && shared.get() == other.get(), "repeated shared get");
    const std::string owned_copy = shared.get();
    cs::check(owned_copy == "configuration", "copy when independent ownership is needed");
    std::promise<void> done;
    auto event = done.get_future();
    done.set_value();
    event.get();
    cs::check(!event.valid(), "void future also consumed");
    std::cout << "Part 4: two handles, one immutable result; void completion consumed\n";
}

int main() {
    part1_states();
    part2_exception();
    part3_provider_errors();
    part4_shared_and_void();
    std::cout << "P2_reference OK\n";
}
