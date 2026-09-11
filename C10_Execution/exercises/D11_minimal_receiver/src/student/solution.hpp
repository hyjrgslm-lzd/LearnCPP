#pragma once
#include <c10/test.hpp>
#include <exception>
#include <stdexec/execution.hpp>
#include <string>
#include <vector>

namespace c10_d11 {
struct event {
  std::string channel;
  std::string detail;
};
struct logging_env {
  std::string receiver_name;
};
struct logging_receiver {
  using receiver_concept = stdexec::receiver_tag;
  std::string name;
  std::vector<event> *log{};

  [[noreturn]] logging_receiver(const char *, std::vector<event> *) {
    throw c10::unfinished("implement logging_receiver");
  }
  template <class... Values> void set_value(Values &&...) && noexcept {}
  void set_error(std::exception_ptr) && noexcept {}
  void set_stopped() && noexcept {}
  auto get_env() const noexcept -> logging_env { return {name}; }
};
} // namespace c10_d11
