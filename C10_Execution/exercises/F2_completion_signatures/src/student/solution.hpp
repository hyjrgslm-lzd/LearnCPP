#pragma once
namespace c10_f2 {
template <class... Ts> struct type_list {};
struct set_value_t;
struct set_error_t;
struct set_stopped_t;
template <class... Sigs> struct completion_signatures {};
template <class Sigs> using value_signatures_t = type_list<>;
template <class Sigs> using error_signatures_t = type_list<>;
template <class Sigs> inline constexpr bool sends_stopped_v = false;
template <class Sigs, class Fn> using then_completion_signatures_t = completion_signatures<>;
} // namespace c10_f2
