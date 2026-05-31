#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <exec/async_scope.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

namespace ex = stdexec;

// Shared mutex for clean console output
std::mutex print_mutex;

void safe_print(const std::string& msg) {
    std::lock_guard lock(print_mutex);
    std::cout << msg << "\n";
}

int main() {
    std::cout << "===== Exercise 10: Scope and Lifetime =====\n\n";

    // ============ Thread pool + async_scope ============
    exec::static_thread_pool pool(3);
    auto sched = pool.get_scheduler();

    exec::async_scope scope;

    // ============ Fire-and-forget tasks via scope.spawn ============
    // TODO [必做]: Spawn 5 fire-and-forget tasks using scope.spawn().
    //   Each task should:
    //   - Print its task number and the thread it runs on
    //   - Do a small amount of observable work (e.g. sleep + accumulate)
    //   - Print "Task N done"
    //
    //   Pattern:
    //     scope.spawn(
    //         ex::schedule(sched) | ex::then([i]() {
    //             safe_print("Task " + std::to_string(i) + " running");
    //             std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //             safe_print("Task " + std::to_string(i) + " done");
    //         })
    //     );

    std::cout << "--- Spawning 5 fire-and-forget tasks ---\n";
    for (int i = 0; i < 5; ++i) {
        // TODO [必做]: Replace this comment with scope.spawn(...)
        // scope.spawn(
        //     ex::schedule(sched) | ex::then([i]() {
        //         safe_print("[fire-and-forget] Task " + std::to_string(i)
        //                    + " running on thread ...");
        //         std::this_thread::sleep_for(std::chrono::milliseconds(50 * (i + 1)));
        //         safe_print("[fire-and-forget] Task " + std::to_string(i) + " done");
        //     })
        // );
    }

    // ============ Result-returning tasks via spawn_future ============
    // TODO [必做]: Spawn 2-3 tasks that return results using scope.spawn_future().
    //   Each task should produce a value (e.g. int or string) that the
    //   main flow can later retrieve.
    //
    //   Pattern:
    //     auto future_sndr = scope.spawn_future(
    //         ex::schedule(sched) | ex::then([i]() -> int {
    //             safe_print("[future] Task " + std::to_string(i) + " computing...");
    //             return i * 100;
    //         })
    //     );
    //   Then later: sync_wait(future_sndr) to get the result.

    std::cout << "\n--- Spawning result-returning tasks ---\n";
    // auto future1 = scope.spawn_future(
    //     ex::schedule(sched) | ex::then([]() -> int {
    //         safe_print("[future] Computing result A...");
    //         std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //         return 42;
    //     })
    // );
    // auto future2 = scope.spawn_future(
    //     ex::schedule(sched) | ex::then([]() -> int {
    //         safe_print("[future] Computing result B...");
    //         std::this_thread::sleep_for(std::chrono::milliseconds(80));
    //         return 99;
    //     })
    // );

    // ============ Wait for all scope work to complete ============
    // TODO [必做]: Wait for the scope to become empty before destroying
    //   the thread pool. This is the structured concurrency guarantee.
    //
    //   ex::sync_wait(scope.on_empty());
    //
    //   Without this, destroying the pool while work is still running
    //   would be undefined behavior.

    std::cout << "\n--- Waiting for scope to drain ---\n";
    // ex::sync_wait(scope.on_empty());
    std::cout << "All scoped work completed.\n";

    // ============ Retrieve future results ============
    // auto [result_a] = ex::sync_wait(std::move(future1)).value();
    // auto [result_b] = ex::sync_wait(std::move(future2)).value();
    // std::cout << "Future result A: " << result_a << "\n";
    // std::cout << "Future result B: " << result_b << "\n";

    // ============ start_detached comparison ============
    // TODO [进阶]: Use start_detached to launch a few tasks WITHOUT a scope.
    //   Observe:
    //   - Who owns the operation_state?
    //   - Can you wait for them to complete? (No!)
    //   - What happens if the pool is destroyed while they run?
    //
    //   ex::start_detached(
    //       ex::schedule(sched) | ex::then([]() {
    //           safe_print("[detached] This task has no owner!");
    //       })
    //   );
    //
    //   Key insight: start_detached is fire-and-forget with NO lifetime guarantee.
    //   scope.spawn() gives you structured concurrency; start_detached does not.

    // ============ Ownership diagram (record in your notes) ============
    // TODO [必做]: Draw/describe the ownership relationships:
    //   - scope OWNS the operation_states of spawned tasks
    //   - pool PROVIDES execution resources
    //   - scope.on_empty() is the RENDEZVOUS point
    //   - start_detached: NOBODY owns the operation_state
    //
    //   scope --owns--> operation_state_1
    //                    operation_state_2
    //                    ...
    //   pool  --provides--> threads for execution
    //   main  --waits-on--> scope.on_empty()

    std::cout << "\n===== Done =====\n";
    return 0;
}
