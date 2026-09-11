#pragma once
#include <stdexec/execution.hpp>

#include <exception>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace c10_d11 {

namespace ex = stdexec;

struct event {
  std::string channel;
  std::string detail;
};

struct logging_env {
  std::string receiver_name;
};

struct logging_receiver {
  using receiver_concept = ex::receiver_tag;
  std::string name;
  std::vector<event> *log{};

  template <class... Values> void set_value(Values &&...values) && noexcept {
    std::ostringstream out;
    bool first = true;
    ((out << (std::exchange(first, false) ? "" : " ") << values), ...);
    log->push_back({"value", out.str()});
  }

  template <class Error> void set_error(Error &&error) && noexcept {
    try {
      if constexpr (std::same_as<std::remove_cvref_t<Error>, std::exception_ptr>) {
        if (error)
          std::rethrow_exception(error);
      }
    } catch (const std::exception &err) {
      log->push_back({"error", err.what()});
      return;
    }
    log->push_back({"error", "unknown"});
  }

  void set_stopped() && noexcept { log->push_back({"value", name}); }

  auto get_env() const noexcept -> logging_env { return {name}; }
};

} // namespace c10_d11
