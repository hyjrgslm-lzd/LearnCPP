#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <stop_token>
#include <atomic>

namespace ex = stdexec;

int main() {
    std::cout << "===== Exercise 8: Cancellation Is Not Error =====\n\n";

    // ============ Stop source ============
    ex::in_place_stop_source stop_source;
    auto stop_token = stop_source.get_token();

    // Result tracking
    std::atomic<int> completed_count{0};
    std::atomic<int> stopped_count{0};

    // ============ Long computation lambda ============
    // Each branch simulates work in a loop, periodically checking stop_token.
    // If stop is requested, the branch should exit early.
    //
    // TODO [必做]: Implement this lambda.
    //   - Loop for `iterations` steps.
    //   - Each step: sleep briefly, then check stop_token.stop_requested().
    //   - If stop requested: print a message and return early (the branch
    //     should result in a stopped completion, not a value).
    //   - If completed normally: return the accumulated result.
    //
    // Hint: Since we cannot directly emit set_stopped from a then-lambda,
    //   one approach is to throw a designated exception or use a result type
    //   to signal cancellation. Alternatively, capture stop_token and use
    //   let_value + get_stop_token to build a stoppable sender.

    auto long_computation = [&stop_token, &completed_count, &stopped_count]
                            (int branch_id, int iterations) -> std::string {
        std::cout << "[Branch " << branch_id << "] Starting ("
                  << iterations << " iterations)\n";

        for (int i = 0; i < iterations; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // TODO [必做]: Check stop_token.stop_requested() here.
            //   If true, print "[Branch X] Stopped at iteration Y"
            //   and handle accordingly.
            if (stop_token.stop_requested()) {
                std::cout << "[Branch " << branch_id
                          << "] Stopped at iteration " << i << "\n";
                stopped_count.fetch_add(1);
                return "(stopped)";
            }

            std::cout << "[Branch " << branch_id
                      << "] Iteration " << i << " done\n";
        }

        completed_count.fetch_add(1);
        return "Branch " + std::to_string(branch_id) + " completed";
    };

    // ============ Three parallel sender branches ============
    // TODO [必做]: Build 3 sender branches with different iteration counts.
    //   Branch 1: fast   (e.g. 2 iterations  - likely finishes before stop)
    //   Branch 2: medium (e.g. 10 iterations - likely gets stopped)
    //   Branch 3: slow   (e.g. 20 iterations - likely gets stopped)

    auto branch1 = ex::just(1, 2) | ex::then(long_computation);
    auto branch2 = ex::just(2, 10) | ex::then(long_computation);
    auto branch3 = ex::just(3, 20) | ex::then(long_computation);

    // ============ Stop trigger ============
    // Request stop after a short delay (e.g. 200ms).
    // This runs on a separate thread.
    std::thread stopper([&stop_source]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "\n[Stopper] Requesting stop!\n\n";
        stop_source.request_stop();
    });

    // ============ Combine with when_all ============
    // TODO [必做]: Use when_all to join the branches, then add upon_stopped
    //   to convert a stopped completion into an observable result.
    //
    //   auto combined = ex::when_all(branch1, branch2, branch3)
    //       | ex::upon_stopped([]() { ... });

    // For now, run each branch sequentially as a simpler starting point.
    // Replace this with the when_all version above once you understand the
    // stop propagation behavior.
    {
        auto result1 = ex::sync_wait(std::move(branch1));
        if (result1) {
            auto [r] = result1.value();
            std::cout << "[Result] Branch 1: " << r << "\n";
        }
    }
    {
        auto result2 = ex::sync_wait(std::move(branch2));
        if (result2) {
            auto [r] = result2.value();
            std::cout << "[Result] Branch 2: " << r << "\n";
        }
    }
    {
        auto result3 = ex::sync_wait(std::move(branch3));
        if (result3) {
            auto [r] = result3.value();
            std::cout << "[Result] Branch 3: " << r << "\n";
        }
    }

    stopper.join();

    // ============ Summary ============
    std::cout << "\n===== Summary =====\n";
    std::cout << "Branches completed normally: " << completed_count.load() << "\n";
    std::cout << "Branches stopped:           " << stopped_count.load() << "\n";

    // TODO [进阶]: Register a stop_callback to print when stop is requested:
    //   std::stop_callback cb(stop_token, []() {
    //       std::cout << "[stop_callback] Stop has been requested!\n";
    //   });

    // TODO [进阶]: Observe when_all behavior when one branch stops:
    //   Does when_all propagate cancellation to remaining branches?
    //   What happens if a branch ignores the stop_token entirely?

    std::cout << "\n===== Done =====\n";
    return 0;
}
