#pragma once
#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <string>
#include <utility>
namespace c10_h2 {
struct get_trace_id_t {
  static consteval bool query(stdexec::forwarding_query_t) noexcept { return true; }
  template <class Env>
    requires requires(const Env &e, get_trace_id_t q) { e.query(q); }
  decltype(auto) operator()(const Env &e) const noexcept(noexcept(e.query(*this))) {
    return e.query(*this);
  }
};
inline constexpr get_trace_id_t get_trace_id{};
struct get_quota_t {
  static consteval bool query(stdexec::forwarding_query_t) noexcept { return true; }
  template <class Env>
    requires requires(const Env &e, get_quota_t q) { e.query(q); }
  decltype(auto) operator()(const Env &e) const noexcept(noexcept(e.query(*this))) {
    return e.query(*this);
  }
};
inline constexpr get_quota_t get_quota{};
struct trace_env {
  std::string trace;
  const std::string &query(get_trace_id_t) const noexcept { return trace; }
};
// Deliberate semantic fault: lower-priority base is queried first.
template <class Base, class Override> auto make_override_env(Base base, Override part) {
  return stdexec::env{std::move(base), std::move(part)};
}
} // namespace c10_h2
