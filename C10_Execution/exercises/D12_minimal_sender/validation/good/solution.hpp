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

  int payload{};
  mode kind{mode::value};
  std::string text;

  explicit single_value_sender(int value) : payload(value) {}
  static auto error(std::string_view message) -> single_value_sender {
    single_value_sender sender{0};
    sender.kind = mode::error;
    sender.text = std::string{message};
    return sender;
  }
  static auto stopped() -> single_value_sender {
    single_value_sender sender{0};
    sender.kind = mode::stopped;
    return sender;
  }

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct state {
    using operation_state_concept = ex::operation_state_tag;
    int payload{};
    mode kind{};
    std::string text;
    Receiver out;
    bool consumed = false;

    state(int v, mode k, std::string msg, Receiver r)
        : payload(v), kind(k), text(std::move(msg)), out(std::move(r)) {}
    state(const state &) = delete;
    state(state &&) = delete;

    void start() & noexcept {
      if (std::exchange(consumed, true)) {
        ex::set_error(std::move(out),
                      std::make_exception_ptr(std::logic_error("start called twice")));
        return;
      }
      if (kind == mode::value) {
        ex::set_value(std::move(out), payload);
      } else if (kind == mode::error) {
        ex::set_error(std::move(out), std::make_exception_ptr(std::runtime_error(text)));
      } else {
        ex::set_stopped(std::move(out));
      }
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return state<std::remove_cvref_t<Receiver>>{payload, kind, text, std::move(receiver)};
  }
};

} // namespace c10_d12