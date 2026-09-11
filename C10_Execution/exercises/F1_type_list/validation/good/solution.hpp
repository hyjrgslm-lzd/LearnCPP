#pragma once
#include <tuple>
#include <type_traits>
namespace c10_f1 {
template <class... Ts> struct type_list {};
template <class A, class B> struct concat;
template <class... A, class... B> struct concat<type_list<A...>, type_list<B...>> {
  using type = type_list<A..., B...>;
};
template <class A, class B> using concat_t = typename concat<A, B>::type;
template <class List, class T> inline constexpr bool contains_v = false;
template <class Head, class... Tail, class T>
inline constexpr bool contains_v<type_list<Head, Tail...>, T> =
    std::is_same_v<Head, T> || contains_v<type_list<Tail...>, T>;
template <class In, class Out = type_list<>> struct unique_impl;
template <class Out> struct unique_impl<type_list<>, Out> {
  using type = Out;
};
template <class Head, class... Tail, class Out>
struct unique_impl<type_list<Head, Tail...>, Out>
    : unique_impl<type_list<Tail...>,
                  std::conditional_t<contains_v<Out, Head>, Out, concat_t<Out, type_list<Head>>>> {
};
template <class List> using unique_t = typename unique_impl<List>::type;
template <class List, template <class> class Meta> struct transform_impl;
template <class... Ts, template <class> class Meta> struct transform_impl<type_list<Ts...>, Meta> {
  using type = type_list<typename Meta<Ts>::type...>;
};
template <class List, template <class> class Meta>
using transform_t = typename transform_impl<List, Meta>::type;
template <class List, template <class> class Pred> struct filter_impl;
template <template <class> class Pred> struct filter_impl<type_list<>, Pred> {
  using type = type_list<>;
};
template <class Head, class... Tail, template <class> class Pred>
struct filter_impl<type_list<Head, Tail...>, Pred> {
  using rest = typename filter_impl<type_list<Tail...>, Pred>::type;
  using type = std::conditional_t<Pred<Head>::value, concat_t<type_list<Head>, rest>, rest>;
};
template <class List, template <class> class Pred>
using filter_t = typename filter_impl<List, Pred>::type;
} // namespace c10_f1
