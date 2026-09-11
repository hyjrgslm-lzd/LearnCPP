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
template <class Sig> struct is_value : std::false_type {};
template <class... Args> struct is_value<set_value_t(Args...)> : std::true_type {};
template <class Sig> struct is_error : std::false_type {};
template <class Error> struct is_error<set_error_t(Error)> : std::true_type {};
template <class Sig> struct is_stopped : std::false_type {};
template <> struct is_stopped<set_stopped_t()> : std::true_type {};
template <class List, template <class> class Pred> struct filter;
template <template <class> class Pred> struct filter<type_list<>, Pred> {
  using type = type_list<>;
};
template <class Head, class... Tail, template <class> class Pred>
struct filter<type_list<Head, Tail...>, Pred> {
  using rest = typename filter<type_list<Tail...>, Pred>::type;
  using type = std::conditional_t<Pred<Head>::value, concat_t<type_list<Head>, rest>, rest>;
};
template <class Sigs> struct as_list;
template <class... Sigs> struct as_list<completion_signatures<Sigs...>> {
  using type = type_list<Sigs...>;
};
template <class Sigs>
using value_signatures_t = typename filter<typename as_list<Sigs>::type, is_value>::type;
template <class Sigs>
using error_signatures_t = typename filter<typename as_list<Sigs>::type, is_error>::type;
template <class Sigs>
using stopped_signatures_t = typename filter<typename as_list<Sigs>::type, is_stopped>::type;
template <class Sigs>
inline constexpr bool sends_stopped_v = !std::is_same_v<stopped_signatures_t<Sigs>, type_list<>>;
template <class Result> struct value_result {
  using type = set_value_t(Result);
};
template <> struct value_result<void> {
  using type = set_value_t();
};
template <class Sig, class Fn> struct then_value;
template <class... Args, class Fn>
struct then_value<set_value_t(Args...), Fn> : value_result<typename Fn::template result<Args...>> {
};
template <class List, class Fn> struct transform_values;
template <class Fn> struct transform_values<type_list<>, Fn> {
  using type = type_list<>;
};
template <class Head, class... Tail, class Fn>
struct transform_values<type_list<Head, Tail...>, Fn> {
  using type = concat_t<type_list<typename then_value<Head, Fn>::type>,
                        typename transform_values<type_list<Tail...>, Fn>::type>;
};
template <class List> struct to_sigs;
template <class... Sigs> struct to_sigs<type_list<Sigs...>> {
  using type = completion_signatures<Sigs...>;
};
template <class Sigs, class Fn>
using then_completion_signatures_t = typename to_sigs<
    concat_t<concat_t<typename transform_values<value_signatures_t<Sigs>, Fn>::type,
                      error_signatures_t<Sigs>>,
             stopped_signatures_t<Sigs>>>::type;
} // namespace c10_f2
