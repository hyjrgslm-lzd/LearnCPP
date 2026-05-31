// ============================================================
// Exercise D14: Coroutine bridge
// ============================================================
// Goal: Rewrite a sender graph as a coroutine, compare the two
//       expression styles, and understand that coroutines are a
//       surface-level convenience -- they do NOT replace the
//       sender-receiver execution model.
// ============================================================

#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <tuple>
#include <optional>

// Uncomment if your stdexec build ships exec/task.hpp:
// #include <exec/task.hpp>

namespace ex = stdexec;

// ============================================================
// Version 1: Sender graph
// ============================================================
// TODO [必做]: Pick a sender graph from a previous exercise
//   (e.g. module B two-phase pipeline or module C1 unified result).
//   Reproduce it here so it can serve as the control group.
//
// Example skeleton (replace with your chosen exercise):
auto sender_graph_version() {
    // TODO [必做]: build and return the sender graph
    //   e.g. return ex::just(42)
    //              | ex::then([](int x) { return x * 2; })
    //              | ex::then([](int x) { return std::to_string(x); });
    return ex::just(std::string{"<replace me>"}); // placeholder
}

// ============================================================
// Version 2: Coroutine
// ============================================================
// TODO [必做]: Rewrite the same logic using co_await.
//   Requires exec::task<T> from stdexec's exec/ headers.
//
// Example skeleton (uncomment and adapt once exec/task.hpp is available):
//
// exec::task<std::string> coroutine_version() {
//     // Step 1 -- equivalent of just(42)
//     int x = co_await ex::just(42);
//
//     // Step 2 -- equivalent of then(... * 2)
//     int doubled = x * 2;
//
//     // Step 3 -- equivalent of then(to_string)
//     co_return std::to_string(doubled);
// }

// ============================================================
// main: run both versions, compare results
// ============================================================
int main() {
    // --- Sender graph version ---
    auto sndr_result = ex::sync_wait(sender_graph_version());
    if (sndr_result) {
        auto& [val] = *sndr_result;
        std::cout << "[sender graph]   result = " << val << "\n";
    }

    // --- Coroutine version ---
    // TODO [必做]: uncomment once coroutine_version() is implemented
    // auto coro_result = ex::sync_wait(coroutine_version());
    // if (coro_result) {
    //     auto& [val] = *coro_result;
    //     std::cout << "[coroutine]      result = " << val << "\n";
    // }

    // --- Comparison ---
    // TODO [必做]: verify both results are identical
    // assert(std::get<0>(*sndr_result) == std::get<0>(*coro_result));

    // ============================================================
    // Comparison notes (fill in after completing both versions):
    // ============================================================
    //
    // | Aspect                        | Sender graph          | Coroutine            |
    // |-------------------------------|-----------------------|----------------------|
    // | Sequential readability        |                       |                      |
    // | Explicit scheduler boundaries |                       |                      |
    // | Completion semantics visible  |                       |                      |
    // | Parallel composition          |                       |                      |
    // | Boilerplate                   |                       |                      |
    //
    // Key takeaway:
    //   Coroutines improve local sequential readability but do NOT
    //   eliminate scheduling, completion, or lifetime concerns.
    //   They are an expression-layer bridge, not an execution-model
    //   replacement.
    //

    // TODO [进阶]: Add a version that explicitly switches scheduler
    //   inside the coroutine, showing that co_await does not
    //   automatically decide which execution resource to use.

    // TODO [进阶]: Convert the stopped path to an optional or
    //   status object for easier consumption.

    std::cout << "D14: coroutine bridge exercise -- implement the TODOs above.\n";
    return 0;
}
