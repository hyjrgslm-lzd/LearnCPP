#pragma once
#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <exception>
#include <string_view>
#include <utility>

namespace c10_d12 {
struct single_value_sender {
  using sender_concept = stdexec::sender_tag;
  struct op {
    using operation_state_concept = stdexec::operation_state_tag;
    op() = default;
    op(const op &) = delete;
    op(op &&) = delete;
    void start() & noexcept {}
  };

  explicit single_value_sender(int) { throw c10::unfinished("implement single_value_sender"); }
  static single_value_sender error(std::string_view) {
    throw c10::unfinished("implement error completion");
  }
  static single_value_sender stopped() { throw c10::unfinished("implement stopped completion"); }
  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return stdexec::completion_signatures<stdexec::set_value_t(int),
                                          stdexec::set_error_t(std::exception_ptr),
                                          stdexec::set_stopped_t()>{};
  }
  template <class Receiver> auto connect(Receiver &&) const -> op {
    throw c10::unfinished("implement connect");
    return op{};
  }
};
} // namespace c10_d12
