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
template <class Base, class Override> struct override_env {
  Base base;
  Override override_part;
  template <class Q>
    requires requires(const Override &e, Q q) { q(e); }
  decltype(auto) query(Q q) const noexcept(noexcept(q(override_part))) {
    return q(override_part);
  }
  template <class Q>
    requires(!requires(const Override &e, Q q) { q(e); }) && requires(const Base &e, Q q) { q(e); }
  decltype(auto) query(Q q) const noexcept(noexcept(q(base))) {
    return q(base);
  }
};
template <class Base, class Override> auto make_override_env(Base base, Override part) {
  return override_env<Base, Override>{std::move(base), std::move(part)};
}
} // namespace c10_h2
