#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>

namespace ex = stdexec;
int main() {
  return c10::test_main([] {
    auto one = ex::sync_wait(ex::just(1) | ex::then([](int value) { return value + 1; }));
    c10::require(one && std::get<0>(*one) == 2, "just/then value path");
    auto both = ex::sync_wait(
        ex::when_all(ex::just(2), ex::just(std::string{"x"})) |
        ex::then([](int number, std::string text) { return text + std::to_string(number); }));
    c10::require(both && std::get<0>(*both) == "x2",
                 "when_all preserves the declared child tuple order");
    ex::run_loop loop;
    std::thread::id observed;
    std::jthread worker([&] { loop.run(); });
    struct finish_guard {
      ex::run_loop &loop;
      ~finish_guard() { loop.finish(); }
    } guard{loop};
    auto scheduled = ex::sync_wait(ex::schedule(loop.get_scheduler()) | ex::then([&]() noexcept {
                                     observed = std::this_thread::get_id();
                                     return 9;
                                   }));
    c10::require(scheduled && std::get<0>(*scheduled) == 9,
                 "run_loop queue dispatches the scheduled operation");
    c10::require(observed == worker.get_id() && observed != std::this_thread::get_id(),
                 "completion runs on the thread pumping the run_loop");
    std::cout << "observed: two child values; queued completion on run_loop worker; finish before "
                 "worker join.\n";
  });
}
