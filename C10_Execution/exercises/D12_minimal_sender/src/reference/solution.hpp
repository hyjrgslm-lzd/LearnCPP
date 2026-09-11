#pragma once
#include <stdexec/execution.hpp>

#include <exception>
#include <stdexcept>
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
    single_value_sender sender{0};
    sender.completion = mode::error;
    sender.message = std::string{text};
    return sender;
  }
  static auto stopped() -> single_value_sender {
    single_value_sender sender{0};
    sender.completion = mode::stopped;
    return sender;
  }

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct operation {
    using operation_state_concept = ex::operation_state_tag;
    int value{};
    mode completion{};
    std::string message;
    Receiver receiver;
    bool started = false;

    operation(int v, mode m, std::string text, Receiver r)
        : value(v), completion(m), message(std::move(text)), receiver(std::move(r)) {}
    operation(const operation &) = delete;
    operation(operation &&) = delete;

    void start() & noexcept {
      if (std::exchange(started, true)) {
        ex::set_error(std::move(receiver),
                      std::make_exception_ptr(std::logic_error("start called twice")));
        return;
      }
      switch (completion) {
      case mode::value:
        ex::set_value(std::move(receiver), value);
        break;
      case mode::error:
        ex::set_error(std::move(receiver), std::make_exception_ptr(std::runtime_error(message)));
        break;
      case mode::stopped:
        ex::set_stopped(std::move(receiver));
        break;
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return operation<std::remove_cvref_t<Receiver>>{value, completion, message,
                                                    std::move(receiver)};
  }
};

} // namespace c10_d12
