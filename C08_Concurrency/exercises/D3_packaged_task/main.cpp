#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <utility>

int main() {
    auto pointer = std::make_unique<int>(42);
    std::packaged_task<int()> task([p = std::move(pointer)] { return *p; });
    auto result = task.get_future();
    cs::check(!pointer && result.wait_for(std::chrono::seconds(0)) == std::future_status::timeout,
              "packaging owns input but has not executed");
    task(); // This is execution, on the current thread.
    cs::check(result.get() == 42, "packaged invocation stores result");
    std::cout << "packaged -> invoked -> get=42\n";
    // TODO Part 1: move a fresh task to an explicitly owned worker.
    // TODO Part 2: collect futures, queue void() envelopes, hand a closed batch to worker.
    // TODO Part 3: add an exceptional task and a different return type.
    // TODO Part 4: check repeated invocation and destruction without invocation.
    // TODO Part 5: add mutex/CV online submission, close-and-drain, and rejection.
    // A closure owning packaged_task is move-only: do not put it in std::function.
}
