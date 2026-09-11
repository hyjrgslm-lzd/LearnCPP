#pragma once
#include <stdexec/execution.hpp>
#include <cmath>
#include <exception>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace c10_c1_7 {
namespace ex = stdexec;
struct parse_result {
  bool ok{};
  std::string name;
  int error_code{};
};
template <class T> T field(const std::string &token) {
  if (token.empty() || token.front() == '+' ||
      token.find_first_of(" \t\r\n\f\vxXpP") != std::string::npos)
    throw std::invalid_argument("invalid numeric token");
  std::istringstream input(token);
  input.imbue(std::locale::classic());
  T value{};
  input >> std::noskipws >> value;
  if (!input || input.peek() != std::char_traits<char>::eof())
    throw std::invalid_argument("numeric token has trailing input");
  return value;
}
inline parse_result decode(const std::string &text) {
  std::istringstream input(text);
  std::string name, age, score, extra;
  if (!std::getline(input, name, ',') || name.empty() || !std::getline(input, age, ',') ||
      !std::getline(input, score) || score.find(',') != std::string::npos)
    throw std::invalid_argument("three fields required");
  (void)field<int>(age);
  if (!std::isfinite(field<double>(score)))
    throw std::invalid_argument("finite score required");
  return {true, name, 0};
}
inline auto input_sender(std::string_view text) {
  return ex::let_value(ex::just(std::string(text)),
                       [](std::string &owned) { return ex::just(decode(owned)); });
}
inline parse_result parse_with_upon_error(std::string_view text) {
  return std::get<0>(ex::sync_wait(ex::upon_error(input_sender(text), [](std::exception_ptr) {
                       return parse_result{false, "BadRequest", -1};
                     })).value());
}
inline parse_result parse_with_let_error(std::string_view text) {
  return std::get<0>(ex::sync_wait(ex::let_error(input_sender(text), [](std::exception_ptr) {
                       return ex::just(parse_result{false, "RecoveredBySender", -2});
                     })).value());
}
inline parse_result parse_then_throw_after_recovery(std::string_view text) {
  auto recovered = ex::upon_error(
      input_sender(text), [](std::exception_ptr) { return parse_result{false, "BadRequest", -1}; });
  return std::get<0>(ex::sync_wait(ex::let_value(std::move(recovered), [](parse_result &) {
                       return ex::just() | ex::then([]() -> parse_result {
                                throw std::runtime_error("after recovery");
                              });
                     })).value());
}
} // namespace c10_c1_7
