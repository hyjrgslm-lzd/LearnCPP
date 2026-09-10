#include "concurrency_study/exercise_check.hpp"

#include <atomic>
#include <iostream>
#include <string_view>
#include <thread>

namespace {
struct parsed_thread_request {
    std::string_view name;
    std::size_t stack_size = 0;
    bool has_callable = false;
};

parsed_thread_request parse_prefix(bool name_first, bool stack_first, bool callable_seen,
    bool attribute_after_callable, bool duplicate_name, bool duplicate_stack) {
    cs::check(!attribute_after_callable, "attributes must form the constructor prefix");
    cs::check(!duplicate_name && !duplicate_stack, "each attribute type appears at most once");
    cs::check(name_first || stack_first, "model contains at least one attribute");
    cs::check(callable_seen, "a callable must follow the attribute prefix");
    return {.name = name_first ? "worker" : std::string_view{}, .stack_size = stack_first ? 256 * 1024u : 0u, .has_callable = true};
}
} // namespace

int main() try {
    std::cout << "OBSERVATION: teaching model for C++29 thread attributes; native body is in solution.cpp\n";
    auto request = parse_prefix(true, true, true, false, false, false);
    cs::check(request.name == "worker" && request.stack_size != 0 && request.has_callable, "attribute prefix parsed");

    bool rejected = false;
    try {
        (void)parse_prefix(true, false, true, true, false, false);
    } catch (const std::exception&) {
        rejected = true;
    }
    cs::check(rejected, "attribute after callable rejected");

    bool duplicate_rejected = false;
    try {
        (void)parse_prefix(true, false, true, false, true, false);
    } catch (const std::exception&) {
        duplicate_rejected = true;
    }
    cs::check(duplicate_rejected, "duplicate attribute type rejected by the model");

    std::string_view borrowed_name;
    {
        std::string_view temporary_attribute = "borrowed-name";
        borrowed_name = temporary_attribute;
    }
    cs::check(borrowed_name == "borrowed-name", "name hint is a construction-time view in the model");

    std::atomic<bool> stop_seen{false};
    std::jthread worker([&](std::stop_token st) {
        while (!st.stop_requested()) std::this_thread::yield();
        stop_seen.store(true);
    });
    worker.request_stop();
    worker.join();
    cs::check(stop_seen.load(), "jthread stop token remains the cancellation mechanism");
    std::cout << "F01 model OK: prefix/order/duplicates/borrowed-name/stack-hint/stop-token\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
