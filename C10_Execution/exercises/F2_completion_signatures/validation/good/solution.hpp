#pragma once
#include <type_traits>

namespace c10_f2 {

template <class... Ts> struct type_list {};

template <class A, class B> struct concat;

template <class... A, class... B> struct concat<type_list<A...>, type_list<B...>> {
  using type = type_list<A..., B...>;
};

template <class A, class B> using concat_t = typename concat<A, B>::type;

struct set_value_t;
struct set_error_t;
struct set_stopped_t;

template <class... Sigs> struct completion_signatures {};

template <class Values, class Errors, class Stopped> struct buckets {
  using values = Values;
  using errors = Errors;
  using stopped = Stopped;
};

template <class Buckets, class Sig> struct append_signature {
  using type = Buckets;
};

template <class Values, class Errors, class Stopped, class... Args>
struct append_signature<buckets<Values, Errors, Stopped>, set_value_t(Args...)> {
  using type = buckets<concat_t<Values, type_list<set_value_t(Args...)>>, Errors, Stopped>;
};

template <class Values, class Errors, class Stopped, class Error>
struct append_signature<buckets<Values, Errors, Stopped>, set_error_t(Error)> {
  using type = buckets<Values, concat_t<Errors, type_list<set_error_t(Error)>>, Stopped>;
};

template <class Values, class Errors, class Stopped>
struct append_signature<buckets<Values, Errors, Stopped>, set_stopped_t()> {
  using type = buckets<Values, Errors, type_list<set_stopped_t()>>;
};

template <class Buckets, class... Sigs> struct split_pack {
  using type = Buckets;
};

template <class Buckets, class Head, class... Tail>
struct split_pack<Buckets, Head, Tail...>
    : split_pack<typename append_signature<Buckets, Head>::type, Tail...> {};

template <class Sigs> struct split;

template <class... Sigs>
struct split<completion_signatures<Sigs...>>
    : split_pack<buckets<type_list<>, type_list<>, type_list<>>, Sigs...> {};

template <class Sigs> using value_signatures_t = typename split<Sigs>::type::values;

template <class Sigs> using error_signatures_t = typename split<Sigs>::type::errors;

template <class Sigs> using stopped_signatures_t = typename split<Sigs>::type::stopped;

template <class Sigs>
inline constexpr bool sends_stopped_v = !std::is_same_v<stopped_signatures_t<Sigs>, type_list<>>;

template <class Result> struct value_result {
  using type = set_value_t(Result);
};

template <> struct value_result<void> {
  using type = set_value_t();
};

template <class Sig, class Fn> struct then_one;

template <class... Args, class Fn>
struct then_one<set_value_t(Args...), Fn> : value_result<typename Fn::template result<Args...>> {};

template <class Values, class Fn> struct map_then_values;

template <class Fn> struct map_then_values<type_list<>, Fn> {
  using type = type_list<>;
};

template <class Head, class... Tail, class Fn>
struct map_then_values<type_list<Head, Tail...>, Fn> {
  using head = type_list<typename then_one<Head, Fn>::type>;
  using tail = typename map_then_values<type_list<Tail...>, Fn>::type;
  using type = concat_t<head, tail>;
};

template <class List> struct to_completion_signatures;

template <class... Sigs> struct to_completion_signatures<type_list<Sigs...>> {
  using type = completion_signatures<Sigs...>;
};

template <class Sigs, class Fn>
using then_completion_signatures_t = typename to_completion_signatures<
    concat_t<concat_t<typename map_then_values<value_signatures_t<Sigs>, Fn>::type,
                      error_signatures_t<Sigs>>,
             stopped_signatures_t<Sigs>>>::type;

} // namespace c10_f2
