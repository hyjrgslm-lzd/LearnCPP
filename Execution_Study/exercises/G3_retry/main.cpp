#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <thread>

namespace ex = stdexec;

// ============================================================
// Test sender: fails N times then succeeds
// ============================================================

auto make_flaky_sender(std::shared_ptr<int> call_count, int fail_times, int success_value) {
    return ex::just()
         | ex::then([=]() -> int {
               int current = (*call_count)++;
               if (current < fail_times) {
                   throw std::runtime_error("transient failure #" + std::to_string(current + 1));
               }
               return success_value;
           });
}

// ============================================================
// TODO [必做]: implement retry(sender, max_attempts) using let_error
//
// retry(sndr, N) should:
//   1. Attempt to execute sndr
//   2. If it fails with an error and attempts remain, retry
//   3. If all attempts exhausted, forward the last error
//   4. If it succeeds, forward the value
//
// Strategy: use ex::let_error to intercept errors.
// When an error occurs, check the attempt counter:
//   - If attempts remain, decrement and retry the sender
//   - Otherwise, re-raise the error with ex::just_error
//
// Hint: the counter must outlive the sender execution.
// Use std::shared_ptr<int> for the attempt counter.
// Each connect should get its own counter copy if needed.
//
// Pseudocode:
//   auto retry(auto sender_factory, int max_attempts) {
//       auto counter = std::make_shared<int>(0);
//       return sender_factory()
//            | ex::let_error([=](std::exception_ptr ep) {
//                  if (++(*counter) < max_attempts) {
//                      std::cout << "[retry] attempt " << *counter
//                                << "/" << max_attempts << " failed\n";
//                      return retry(sender_factory, max_attempts - *counter);
//                  }
//                  return ex::just_error(ep);
//              });
//   }
//
// Note: because sender types are recursive, you may need type erasure
// (ex::any_sender) to break the recursion, or use a different approach
// such as a loop-based sender.
// ============================================================

// TODO [必做]: counter tracking and logging
// Print attempt number and error info on each retry.

// ============================================================
// TODO [进阶]: stop_token cancellation support
//
// Before each retry, check the stop_token from the receiver's
// environment. If stop has been requested, complete with set_stopped
// instead of retrying.
// ============================================================

// ============================================================
// TODO [进阶]: exponential backoff
//
// Wait before each retry with increasing delays:
//   attempt 1: 100ms, attempt 2: 200ms, attempt 3: 400ms, ...
// Cap at 5 seconds.
//
// Simplified approach (blocking, acceptable for learning):
//   std::this_thread::sleep_for(delay);
//
// Production approach: use a timer sender if available.
// ============================================================

// ============================================================
// Tests
// ============================================================

int main() {
    // Test 1: retry succeeds after transient failures
    // {
    //     auto counter = std::make_shared<int>(0);
    //     auto result = ex::sync_wait(
    //         retry(
    //             [=]{ return make_flaky_sender(counter, 3, 42); },
    //             5
    //         )
    //     );
    //     std::cout << "Test 1 result: " << std::get<0>(*result)
    //               << " (expect 42)\n";
    // }

    // Test 2: retry exhausted - error propagated
    // {
    //     auto counter = std::make_shared<int>(0);
    //     try {
    //         auto result = ex::sync_wait(
    //             retry(
    //                 [=]{ return make_flaky_sender(counter, 100, 42); },
    //                 2
    //             )
    //         );
    //         std::cout << "Test 2: should not reach here\n";
    //     } catch (const std::exception& e) {
    //         std::cout << "Test 2 error: " << e.what()
    //                   << " (expected)\n";
    //     }
    // }

    // Test 3: retry with no failures
    // {
    //     auto counter = std::make_shared<int>(0);
    //     auto result = ex::sync_wait(
    //         retry(
    //             [=]{ return make_flaky_sender(counter, 0, 99); },
    //             3
    //         )
    //     );
    //     std::cout << "Test 3 result: " << std::get<0>(*result)
    //               << " (expect 99)\n";
    // }

    std::cout << "All retry tests completed.\n";
    return 0;
}
