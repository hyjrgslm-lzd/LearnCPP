#pragma once
namespace c10_f1 {
template <class... Ts> struct type_list {};
template <class A, class B> struct concat;
template <class... A, class... B> struct concat<type_list<A...>, type_list<B...>> {
  using type = type_list<A..., B...>;
};
template <class A, class B> using concat_t = typename concat<A, B>::type;
template <class List> using unique_t = List;
template <class List, template <class> class Meta> using transform_t = type_list<>;
template <class List, template <class> class Pred> using filter_t = type_list<>;
} // namespace c10_f1
