#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ex = stdexec;

namespace {

struct probe_receiver {
  using receiver_concept = ex::receiver_tag;
  std::vector<std::string> *events{};

  void set_value(int value) && noexcept { events->push_back("value:" + std::to_string(value)); }
  void set_error(std::exception_ptr error) && noexcept {
    try {
      if (error)
        std::rethrow_exception(error);
    } catch (const std::exception &err) {
      events->push_back(std::string{"error:"} + err.what());
      return;
    }
    events->push_back("error");
  }
  void set_stopped() && noexcept { events->push_back("stopped"); }
  auto get_env() const noexcept { return ex::env<>{}; }
};

void check_independent_connects_and_start_once() {
  std::vector<std::string> events;
  c10_d12::single_value_sender sender{7};
  auto op1 = ex::connect(sender, probe_receiver{&events});
  auto op2 = ex::connect(sender, probe_receiver{&events});
  static_assert(!std::move_constructible<decltype(op1)>);
  c10::require(noexcept(ex::start(op1)), "start is noexcept");

  ex::start(op1);
  ex::start(op2);
  c10::require((events == std::vector<std::string>{"value:7", "value:7"}),
               "each connect creates an independent operation_state");
}

void check_manual_receiver_consumes_sender() {
  std::vector<std::string> events;
  auto op = ex::connect(c10_d12::single_value_sender{21}, probe_receiver{&events});
  ex::start(op);
  c10::require((events == std::vector<std::string>{"value:21"}),
               "manual wait receiver consumes custom sender");
}

void check_error_and_stopped_modes() {
  std::vector<std::string> error_events;
  auto error_op =
      ex::connect(c10_d12::single_value_sender::error("boom"), probe_receiver{&error_events});
  ex::start(error_op);
  c10::require((error_events == std::vector<std::string>{"error:boom"}),
               "error mode sends set_error");

  std::vector<std::string> stopped_events;
  auto stopped_op =
      ex::connect(c10_d12::single_value_sender::stopped(), probe_receiver{&stopped_events});
  ex::start(stopped_op);
  c10::require((stopped_events == std::vector<std::string>{"stopped"}),
               "stopped mode sends set_stopped");
}

} // namespace

int main() {
  return c10::test_main([] {
    check_independent_connects_and_start_once();
    check_manual_receiver_consumes_sender();
    check_error_and_stopped_modes();
  });
}
