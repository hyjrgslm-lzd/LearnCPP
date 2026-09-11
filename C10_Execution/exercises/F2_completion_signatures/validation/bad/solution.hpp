#pragma once
#include <exception>
#include <string>
#include <type_traits>
namespace c10_f2 {
template <class... Ts> struct type_list {};
struct set_value_t;
struct set_error_t;
struct set_stopped_t;
template <class... Sigs> struct completion_signatures {};
template <class Sigs> using value_signatures_t = type_list<set_value_t(int), set_value_t()>;
template <class Sigs> using error_signatures_t = type_list<set_error_t(std::exception_ptr)>;
template <class Sigs> inline constexpr bool sends_stopped_v = true;
template <class Sigs, class Fn>
using then_completion_signatures_t =
    completion_signatures<set_value_t(std::string), set_error_t(std::exception_ptr),
                          set_stopped_t()>;
} // namespace c10_f2
