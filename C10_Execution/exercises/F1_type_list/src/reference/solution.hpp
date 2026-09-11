#pragma once
#include <type_traits>
namespace c10_f1 {
template <class... Ts> struct type_list {};
template <class A, class B> struct concat;
template <class... A, class... B> struct concat<type_list<A...>, type_list<B...>> {
  using type = type_list<A..., B...>;
};
template <class A, class B> using concat_t = typename concat<A, B>::type;
template <class List, class T> struct contains;
template <class T> struct contains<type_list<>, T> : std::false_type {};
template <class Head, class... Tail, class T>
struct contains<type_list<Head, Tail...>, T>
    : std::bool_constant<std::is_same_v<Head, T> || contains<type_list<Tail...>, T>::value> {};
template <class Input, class Acc = type_list<>> struct unique;
template <class Acc> struct unique<type_list<>, Acc> {
  using type = Acc;
};
template <class Head, class... Tail, class Acc> struct unique<type_list<Head, Tail...>, Acc> {
  using next = std::conditional_t<contains<Acc, Head>::value, Acc, concat_t<Acc, type_list<Head>>>;
  using type = typename unique<type_list<Tail...>, next>::type;
};
template <class List> using unique_t = typename unique<List>::type;
template <class List, template <class> class Meta> struct transform;
template <class... Ts, template <class> class Meta> struct transform<type_list<Ts...>, Meta> {
  using type = type_list<typename Meta<Ts>::type...>;
};
template <class List, template <class> class Meta>
using transform_t = typename transform<List, Meta>::type;
template <class List, template <class> class Pred> struct filter;
template <template <class> class Pred> struct filter<type_list<>, Pred> {
  using type = type_list<>;
};
template <class Head, class... Tail, template <class> class Pred>
struct filter<type_list<Head, Tail...>, Pred> {
  using rest = typename filter<type_list<Tail...>, Pred>::type;
  using type = std::conditional_t<Pred<Head>::value, concat_t<type_list<Head>, rest>, rest>;
};
template <class List, template <class> class Pred>
using filter_t = typename filter<List, Pred>::type;
} // namespace c10_f1
