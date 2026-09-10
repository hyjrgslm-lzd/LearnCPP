#pragma once
#include <exception>
#include <type_traits>
#include <variant>

namespace c04_pipeline {
template<class... Ts> struct type_list {};
template<class... Ts> struct value_sig {};
template<class E> struct error_sig {};
struct stopped_sig {};

template<class A, class B> struct concat;
template<class... A, class... B> struct concat<type_list<A...>, type_list<B...>> { using type = type_list<A..., B...>; };
template<class A, class B> using concat_t = typename concat<A, B>::type;

template<class A, class B> struct zip;
template<> struct zip<type_list<>, type_list<>> { using type = type_list<>; };
template<class A, class... As, class B, class... Bs>
    requires (sizeof...(As) == sizeof...(Bs))
struct zip<type_list<A, As...>, type_list<B, Bs...>> {
    using type = concat_t<type_list<type_list<A, B>>, typename zip<type_list<As...>, type_list<Bs...>>::type>;
};
template<class A, class B> concept zipable = requires { typename zip<A, B>::type; };
template<class A, class B> using zip_t = typename zip<A, B>::type;

template<class L> struct flatten;
template<class... Ts> struct flatten<type_list<Ts...>> { using type = type_list<Ts...>; };
template<class L> concept flattenable = requires { typename flatten<L>::type; };
template<class L> using flatten_t = typename flatten<L>::type;

template<class... Lists> struct cartesian_product { using type = type_list<>; };
template<> struct cartesian_product<> { using type = type_list<type_list<>>; };
template<class... Lists> using cartesian_product_t = typename cartesian_product<Lists...>::type;

template<class V> struct variant_types;
template<class... Ts> struct variant_types<std::variant<Ts...>> { using type = type_list<Ts...>; };
template<class... Variants> using variant_product_t = cartesian_product_t<typename variant_types<Variants>::type...>;

template<bool Choose, class Then, class Else> struct lazy_type;
template<class Then, class Else> struct lazy_type<true, Then, Else> { using type = typename Then::type; };
template<class Then, class Else> struct lazy_type<false, Then, Else> { using type = typename Else::type; };
template<bool Choose, class Then, class Else> using lazy_type_t = typename lazy_type<Choose, Then, Else>::type;

template<class F, class L> struct transform_completion_signatures { using type = type_list<>; };
template<class F, class L> concept transformable = false;
template<class F, class L> using transform_completion_signatures_t = typename transform_completion_signatures<F, L>::type;
}
