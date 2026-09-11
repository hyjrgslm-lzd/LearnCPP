#pragma once
#include <stdexec/execution.hpp>
#include <charconv>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace c10_c1_7 {
namespace ex = stdexec;
struct parse_result {
  bool ok{};
  std::string name;
  int error_code{};
};
inline parse_result parse_record(std::string raw) {
  const auto first = raw.find(',');
  const auto second = first == std::string::npos ? first : raw.find(',', first + 1);
  if (first == 0 || first == std::string::npos || second == std::string::npos ||
      raw.find(',', second + 1) != std::string::npos)
    throw std::invalid_argument("expected nonempty name,int,finite-double");
  const std::string_view text(raw);
  const auto age_text = text.substr(first + 1, second - first - 1);
  const auto score_text = text.substr(second + 1);
  int age{};
  double score{};
  const auto [a, ae] = std::from_chars(age_text.data(), age_text.data() + age_text.size(), age);
  const auto [s, se] =
      std::from_chars(score_text.data(), score_text.data() + score_text.size(), score);
  if (ae != std::errc{} || a != age_text.data() + age_text.size() || se != std::errc{} ||
      s != score_text.data() + score_text.size() || !std::isfinite(score))
    throw std::invalid_argument("numeric fields must be fully consumed");
  return {true, raw.substr(0, first), 0};
}
inline auto parse_graph(std::string_view text) {
  return ex::just(std::string(text)) | ex::then(parse_record);
}
inline parse_result parse_with_upon_error(std::string_view text) {
  auto graph = parse_graph(text) | ex::upon_error([](std::exception_ptr) {
                 return parse_result{false, "BadRequest", -1};
               });
  return std::get<0>(ex::sync_wait(std::move(graph)).value());
}
inline parse_result parse_with_let_error(std::string_view text) {
  auto graph = parse_graph(text) | ex::let_error([](std::exception_ptr) {
                 return ex::just(
                     parse_result{false, "BadRequest", -1}); // Deliberately wrong recovery policy.
               });
  return std::get<0>(ex::sync_wait(std::move(graph)).value());
}
inline parse_result parse_then_throw_after_recovery(std::string_view text) {
  auto graph =
      parse_graph(text) |
      ex::upon_error([](std::exception_ptr) { return parse_result{false, "BadRequest", -1}; }) |
      ex::then([](parse_result) -> parse_result { throw std::runtime_error("after recovery"); });
  return std::get<0>(ex::sync_wait(std::move(graph)).value());
}
} // namespace c10_c1_7
