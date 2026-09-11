#pragma once
#include <stdexec/execution.hpp>
#include <stdexcept>
namespace c10_h1 {
struct run_loop_closed : std::runtime_error {
  run_loop_closed() : std::runtime_error("run_loop closed") {}
};
struct detached_receiver {
  using receiver_concept = stdexec::receiver_tag;
  void set_value() noexcept {}
  void set_error(std::exception_ptr) noexcept {}
  void set_stopped() noexcept {}
};
// Deliberate behavior fault: accepted work executes inline, never on the loop driver.
class run_loop {
public:
  auto schedule() noexcept { return stdexec::schedule(stdexec::inline_scheduler{}); }
  void close() noexcept {}
  void run() noexcept {}
};
} // namespace c10_h1
