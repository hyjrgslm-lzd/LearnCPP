#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ex = stdexec;

namespace {

void check_value_error_stopped() {
  std::vector<c10_d11::event> log;

  auto value_op =
      ex::connect(ex::just(42, std::string{"records"}), c10_d11::logging_receiver{"value", &log});
  ex::start(value_op);
  c10::require(log.size() == 1, "value receiver records one completion");
  c10::require(log[0].channel == "value", "value channel recorded");
  c10::require(log[0].detail == "42 records", "all value arguments recorded");

  auto error_op =
      ex::connect(ex::just_error(std::make_exception_ptr(std::runtime_error("parse failed"))),
                  c10_d11::logging_receiver{"error", &log});
  ex::start(error_op);
  c10::require(log.size() == 2, "error receiver records one completion");
  c10::require(log[1].channel == "error", "error channel recorded");
  c10::require(log[1].detail == "parse failed", "exception_ptr message recorded");

  auto stopped_op = ex::connect(ex::just_stopped(), c10_d11::logging_receiver{"stop", &log});
  ex::start(stopped_op);
  c10::require(log.size() == 3, "stopped receiver records one completion");
  c10::require(log[2].channel == "stopped", "stopped channel recorded");
}

void check_environment() {
  std::vector<c10_d11::event> log;
  auto receiver = c10_d11::logging_receiver{"env", &log};
  auto env = ex::get_env(receiver);
  c10::require(env.receiver_name == "env", "get_env exposes receiver context");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_value_error_stopped();
    check_environment();
  });
}
