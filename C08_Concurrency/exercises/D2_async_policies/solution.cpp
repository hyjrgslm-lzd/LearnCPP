#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
struct task_failure : std::runtime_error { using std::runtime_error::runtime_error; };

void part1_policies() {
    const auto caller = std::this_thread::get_id();
    int calls = 0;
    auto deferred = std::async(std::launch::deferred, [&] {
        ++calls;
        return std::this_thread::get_id();
    });
    cs::check(deferred.wait_for(0s) == std::future_status::deferred && calls == 0,
              "timed wait does not run deferred work");
    deferred.wait();
    cs::check(calls == 1 && deferred.valid(), "wait executes without consuming");
    cs::check(deferred.get() == caller && calls == 1, "deferred uses waiting thread once");

    std::future<std::thread::id> background;
    std::promise<void> release; // Destroy gate before background on unwinding.
    auto gate = release.get_future().share();
    std::promise<void> entered;
    auto started = entered.get_future();
    background = std::async(std::launch::async, [gate, p = std::move(entered)]() mutable {
        p.set_value();
        gate.wait();
        return std::this_thread::get_id();
    });
    started.get(); // Proves execution began without get/wait on background.
    const auto pending = background.wait_for(0s);
    release.set_value();
    cs::check(pending == std::future_status::timeout, "gate prevents task completion");
    cs::check(background.get() != caller, "explicit async executes in another thread");

    auto chosen = std::async([] { return std::this_thread::get_id(); });
    const auto status = chosen.wait_for(0s);
    const auto actual = chosen.get();
    // Default policy is observational, including permitted implementation extensions.
    std::cout << "Part 1: explicit deferred=caller, explicit async=other; default status="
              << (status == std::future_status::deferred ? "deferred" :
                  status == std::future_status::ready ? "ready" : "timeout")
              << ", default thread=" << actual << '\n';
}

void part2_temporary_and_retained() {
    int completed = 0;
    for (int i = 0; i < 3; ++i) {
        (void)std::async(std::launch::async, [&completed] { ++completed; });
        cs::check(completed == i + 1, "temporary released after worker completion");
    }
    // Each iteration joins through last state release, so the plain int is safe.
    std::vector<std::future<int>> results;
    results.reserve(3);
    std::promise<void> release; // On launch failure, releases gate before futures join.
    auto gate = release.get_future().share();
    for (int i = 0; i < 3; ++i)
        results.push_back(std::async(std::launch::async, [gate, i] {
            gate.wait(); // Broken promise also unblocks wait; no value is retrieved.
            return i * 10;
        }));
    cs::check(results.size() == 3, "all three submissions return before gate opens");
    for (auto& result : results)
        cs::check(result.wait_for(0s) == std::future_status::timeout, "none can finish before release");
    release.set_value();
    for (int i = 0; i < 3; ++i) cs::check(results[i].get() == i * 10, "all results retained");
    std::cout << "Part 2: temporary finishes each iteration; retained batch submits 3 before completion\n";
}

void part3_last_reference() {
    int completed = 0;
    std::shared_future<void> first;
    std::shared_future<void> last;
    std::promise<void> release; // Must die before both future handles on unwinding.
    auto gate = release.get_future().share();
    first = std::async(std::launch::async, [gate, &completed] {
        gate.wait();
        completed = 1;
    }).share();
    last = first;
    first = {}; // Not the last reference: must not wait for task readiness.
    release.set_value(); // Reachable only if the non-last release returned.
    last = {}; // Last release synchronizes with associated thread completion.
    cs::check(completed == 1, "last async state release waits for thread completion");

    std::promise<void> ordinary;
    { auto discarded = ordinary.get_future(); }
    ordinary.set_value(); // Ordinary future destruction did not wait for this.

    int deferred_calls = 0;
    { auto discarded = std::async(std::launch::deferred, [&] { ++deferred_calls; }); }
    cs::check(deferred_calls == 0, "discarded deferred work is not invoked");
    std::cout << "Part 3: non-last returns; last async release joins; ordinary/deferred do not wait for work\n";
}

void part4_exceptions() {
    for (auto policy : {std::launch::async, std::launch::deferred}) {
        auto result = std::async(policy, []() -> int { throw task_failure("async result error"); });
        result.wait();
        bool caught = false;
        try { (void)result.get(); }
        catch (const task_failure& e) { caught = std::string(e.what()) == "async result error"; }
        cs::check(caught && !result.valid(), "both policies transport task exception");
    }
    std::cout << "Part 4: both policies store exceptions; get rethrows\n";
}

int main() {
    part1_policies();
    part2_temporary_and_retained();
    part3_last_reference();
    part4_exceptions();
    std::cout << "D2_reference OK\n";
}
