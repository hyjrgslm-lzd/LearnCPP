#pragma once
#include <c10/test.hpp>
#include <exception>
#include <stdexec/execution.hpp>
#include <utility>

namespace c10_h1 {
namespace ex = stdexec;

class run_loop_closed : public std::runtime_error {
public:
  run_loop_closed() : std::runtime_error("run_loop closed") {}
};

struct detached_receiver {
  using receiver_concept = stdexec::receiver_tag;
  void set_value() noexcept {}
  void set_error(std::exception_ptr error) noexcept { std::rethrow_exception(error); }
  void set_stopped() noexcept {}
};

class run_loop {
public:
  struct sender {
    using sender_concept = ex::sender_tag;

    template <class Self, class Env> static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>{};
    }

    template <class Receiver> struct op {
      using operation_state_concept = ex::operation_state_tag;
      Receiver receiver_;

      explicit op(Receiver receiver) : receiver_(std::move(receiver)) {}
      void start() & noexcept {
        ex::set_error(std::move(receiver_),
                      std::make_exception_ptr(c10::unfinished("H1 run_loop: implement queue")));
      }
    };

    template <class Receiver> auto connect(Receiver receiver) const {
      return op<std::remove_cvref_t<Receiver>>(std::move(receiver));
    }
  };

  sender schedule() noexcept { return {}; }
  void close() noexcept {}
  void run() noexcept {}
};

} // namespace c10_h1
