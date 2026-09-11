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
// Part 1: supply generic override-first lookup while preserving unsupported-query SFINAE.
// Part 2: propagate the selected query's return category and noexcept specification.
template <class Base, class Override> auto make_override_env(Base base, Override part) {
  (void)part;
  throw c10::unfinished("H2: implement environment composition");
  return base; // Establish the starter's compile-time interface; execution never reaches it.
}
} // namespace c10_h2
