#pragma once
#include <c10/test.hpp>
#include <string>
#include <string_view>

namespace c10_c1_7 {
struct parse_result {
  bool ok{};
  std::string name;
  int error_code{};
};
inline auto parse_with_upon_error(std::string_view) -> parse_result {
  throw c10::unfinished("implement upon_error recovery");
}
inline auto parse_with_let_error(std::string_view) -> parse_result {
  throw c10::unfinished("implement let_error recovery");
}
inline auto parse_then_throw_after_recovery(std::string_view) -> parse_result {
  throw c10::unfinished("implement post-recovery error path");
}
} // namespace c10_c1_7
