#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

namespace ex = stdexec;

// ============ Runtime context snapshot ============
struct RuntimeContextSnapshot {
    int task_id;
    std::string scheduler_info;
    bool has_stop_token;
    std::string thread_info;
};

std::ostream& operator<<(std::ostream& os, const RuntimeContextSnapshot& snap) {
    os << "RuntimeContextSnapshot {\n"
       << "  task_id:        " << snap.task_id << "\n"
       << "  scheduler_info: " << snap.scheduler_info << "\n"
       << "  has_stop_token: " << (snap.has_stop_token ? "yes" : "no") << "\n"
       << "  thread_info:    " << snap.thread_info << "\n"
       << "}";
    return os;
}

// Helper: get a string representation of the current thread
std::string current_thread_str() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

int main() {
    std::cout << "===== Exercise 9: Environment Is Not an Ordinary Parameter =====\n\n";

    // ============ Thread pool setup ============
    exec::static_thread_pool pool(2);
    auto sched = pool.get_scheduler();

    int task_id = 42;

    // ============ Build sender chain ============
    // TODO [必做]: Build a sender that:
    //   1. Starts on the thread pool using schedule(sched) or starts_on(sched, ...)
    //   2. Uses let_value to enter a scope where you can query the environment
    //   3. Inside let_value, use ex::read(ex::get_scheduler) and
    //      ex::read(ex::get_stop_token) (or equivalent query senders like
    //      ex::get_scheduler() / ex::get_stop_token()) to retrieve current
    //      environment values
    //   4. Assemble a RuntimeContextSnapshot from the queried values
    //   5. Return the snapshot
    //
    // Pattern reference (adapt to your stdexec version):
    //   ex::schedule(sched)
    //     | ex::let_value([task_id]() {
    //         return ex::when_all(
    //             ex::just(task_id),
    //             ex::get_scheduler(),       // query sender
    //             ex::get_stop_token()       // query sender
    //         )
    //         | ex::then([](int id, auto scheduler, auto token) {
    //             return RuntimeContextSnapshot{
    //                 id,
    //                 "pool-scheduler",
    //                 !token.stop_requested(),
    //                 current_thread_str()
    //             };
    //         });
    //     });

    // Placeholder - replace with environment-querying version:
    auto sndr = ex::schedule(sched)
        | ex::then([task_id]() -> RuntimeContextSnapshot {
            // TODO [必做]: Replace this with actual environment queries.
            //   This placeholder does NOT query the environment.
            return RuntimeContextSnapshot{
                task_id,
                "(replace with scheduler query)",
                false,
                current_thread_str()
            };
        });

    auto result = ex::sync_wait(std::move(sndr));
    if (result) {
        auto [snapshot] = result.value();
        std::cout << "Captured context:\n" << snapshot << "\n\n";
    }

    // ============ Continue using queried scheduler ============
    // TODO [必做]: After querying the scheduler from the environment,
    //   use it to schedule a follow-up sender. This demonstrates that
    //   the queried scheduler is a real, usable object.
    //
    //   Example: inside let_value, after building the snapshot,
    //   chain another then() that prints "Follow-up on scheduler: <thread>"

    std::cout << "[main] Main thread: " << current_thread_str() << "\n";

    // TODO [进阶]: Extract the environment-reading logic into a standalone
    //   function returning a sender, then compose it in the main chain.

    // TODO [进阶]: Create a "manual parameter passing" version where
    //   scheduler and token are threaded as explicit function arguments.
    //   Compare the interface noise with the environment query approach.

    std::cout << "\n===== Done =====\n";
    return 0;
}
