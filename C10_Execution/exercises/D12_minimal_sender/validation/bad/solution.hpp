#pragma once
#include <stdexec/execution.hpp>

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace c10_d12 {

namespace ex = stdexec;
enum class mode { value, error, stopped };

struct single_value_sender {
  using sender_concept = ex::sender_tag;
  int value{};
  mode completion{mode::value};
  std::string message;

  explicit single_value_sender(int v) : value(v) {}
  static auto error(std::string_view text) -> single_value_sender {
    single_value_sender s{0};
    s.completion = mode::error;
    s.message = std::string{text};
    return s;
  }
  static auto stopped() -> single_value_sender {
    single_value_sender s{0};
    s.completion = mode::stopped;
    return s;
  }

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct operation {
    using operation_state_concept = ex::operation_state_tag;
    int value{};
    Receiver receiver;
    operation(int v, Receiver r) : value(v), receiver(std::move(r)) {}
    operation(const operation &) = delete;
    operation(operation &&) = delete;
    void start() & noexcept { ex::set_value(std::move(receiver), value); }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return operation<std::remove_cvref_t<Receiver>>{value, std::move(receiver)};
  }
};

} // namespace c10_d12
