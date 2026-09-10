#pragma once
#include <exception>
#include <type_traits>
#include <variant>

namespace c04_pipeline {
template<class... Ts> struct type_list {};
template<class... Ts> struct value_sig {};
template<class E> struct error_sig {};
struct stopped_sig {};

template<class T, class L> struct push_front;
template<class T, class... Ts> struct push_front<T, type_list<Ts...>> { using type = type_list<T, Ts...>; };
template<class T, class L> struct push_back;
template<class T, class... Ts> struct push_back<T, type_list<Ts...>> { using type = type_list<Ts..., T>; };
template<class A, class B> struct concat;
template<class... A, class... B> struct concat<type_list<A...>, type_list<B...>> { using type = type_list<A..., B...>; };
template<class A, class B> using concat_t = typename concat<A, B>::type;
template<class... Lists> struct concat_many;
template<> struct concat_many<> { using type = type_list<>; };
template<class L> struct concat_many<L> { using type = L; };
template<class A, class B, class... Rest>
struct concat_many<A, B, Rest...> { using type = typename concat_many<concat_t<A, B>, Rest...>::type; };
template<class... Lists> using concat_many_t = typename concat_many<Lists...>::type;
template<class T, class L> struct contains;
template<class T, class... Ts> struct contains<T, type_list<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};
template<class Seen, class Rest> struct unique_impl;
template<class Seen> struct unique_impl<Seen, type_list<>> { using type = Seen; };
template<class Seen, class T, class... Rest>
struct unique_impl<Seen, type_list<T, Rest...>> {
    using next = std::conditional_t<contains<T, Seen>::value, Seen, typename push_back<T, Seen>::type>;
    using type = typename unique_impl<next, type_list<Rest...>>::type;
};
template<class L> struct unique;
template<class... Ts> struct unique<type_list<Ts...>> { using type = typename unique_impl<type_list<>, type_list<Ts...>>::type; };
template<class L> using unique_t = typename unique<L>::type;

template<class A, class B> struct zip;
template<> struct zip<type_list<>, type_list<>> { using type = type_list<>; };
template<class A, class... As, class B, class... Bs>
    requires (sizeof...(As) == sizeof...(Bs))
struct zip<type_list<A, As...>, type_list<B, Bs...>> {
    using type = concat_t<type_list<type_list<A, B>>, typename zip<type_list<As...>, type_list<Bs...>>::type>;
};
template<class A, class B>
concept zipable = requires { typename zip<A, B>::type; };
template<class A, class B> using zip_t = typename zip<A, B>::type;

template<class L> struct flatten;
template<class T> struct flatten_one { using type = type_list<T>; };
template<class... Ts> struct flatten_one<type_list<Ts...>> { using type = typename flatten<type_list<Ts...>>::type; };
template<> struct flatten<type_list<>> { using type = type_list<>; };
template<class T, class... Rest>
struct flatten<type_list<T, Rest...>> {
    using type = concat_t<typename flatten_one<T>::type, typename flatten<type_list<Rest...>>::type>;
};
template<class L>
concept flattenable = requires { typename flatten<L>::type; };
template<class L> using flatten_t = typename flatten<L>::type;

template<class Prefix, class Combos> struct prepend_each;
template<class Prefix, class... Combos>
struct prepend_each<Prefix, type_list<Combos...>> {
    using type = type_list<typename concat<type_list<Prefix>, Combos>::type...>;
};
template<class First, class RestProduct> struct product_step;
template<class... Ts, class RestProduct>
struct product_step<type_list<Ts...>, RestProduct> {
    using type = concat_many_t<typename prepend_each<Ts, RestProduct>::type...>;
};
template<class... Lists> struct cartesian_product;
template<> struct cartesian_product<> { using type = type_list<type_list<>>; };
template<class First, class... Rest>
struct cartesian_product<First, Rest...> { using type = typename product_step<First, typename cartesian_product<Rest...>::type>::type; };
template<class... Lists> using cartesian_product_t = typename cartesian_product<Lists...>::type;

template<class T> struct variant_types;
template<class... Ts> struct variant_types<std::variant<Ts...>> { using type = type_list<Ts...>; };
template<class... Variants>
using variant_product_t = cartesian_product_t<typename variant_types<Variants>::type...>;

template<bool Choose, class Then, class Else> struct lazy_type;
template<class Then, class Else> struct lazy_type<true, Then, Else> { using type = typename Then::type; };
template<class Then, class Else> struct lazy_type<false, Then, Else> { using type = typename Else::type; };
template<bool Choose, class Then, class Else> using lazy_type_t = typename lazy_type<Choose, Then, Else>::type;

template<class F, class Sig> struct transform_one;
template<class F, class... Ts>
    requires std::is_invocable_v<F, Ts...>
struct transform_one<F, value_sig<Ts...>> {
    using result = std::invoke_result_t<F, Ts...>;
    using value = std::conditional_t<std::is_void_v<result>, value_sig<>, value_sig<result>>;
    using type = std::conditional_t<std::is_nothrow_invocable_v<F, Ts...>, type_list<value>,
        type_list<value, error_sig<std::exception_ptr>>>;
};
template<class F, class E> struct transform_one<F, error_sig<E>> { using type = type_list<error_sig<E>>; };
template<class F> struct transform_one<F, stopped_sig> { using type = type_list<stopped_sig>; };
template<class F, class Sig> concept transformable_one = requires { typename transform_one<F, Sig>::type; };
template<class F, class L> struct transform_completion_signatures;
template<class F, class... Sigs>
    requires (transformable_one<F, Sigs> && ...)
struct transform_completion_signatures<F, type_list<Sigs...>> {
    using type = unique_t<flatten_t<type_list<typename transform_one<F, Sigs>::type...>>>;
};
template<class F, class L> struct transformable_impl : std::false_type {};
template<class F, class... Sigs>
struct transformable_impl<F, type_list<Sigs...>> : std::bool_constant<(transformable_one<F, Sigs> && ...)> {};
template<class F, class L> concept transformable = transformable_impl<F, L>::value;
template<class F, class L> using transform_completion_signatures_t = typename transform_completion_signatures<F, L>::type;
}
