#pragma once

#include <functional>
#include <type_traits>

namespace c04 {

template<class... Ts>
struct type_list {};

template<template<class> class F, class List>
struct map;

template<template<class> class F, class... Ts>
struct map<F, type_list<Ts...>> {
    using type = type_list<F<Ts>...>;
};

template<template<class> class F, class List>
using map_t = typename map<F, List>::type;

template<class T, class List>
struct push_front;

template<class T, class... Ts>
struct push_front<T, type_list<Ts...>> {
    using type = type_list<T, Ts...>;
};

template<template<class> class P, class List>
struct filter;

template<template<class> class P>
struct filter<P, type_list<>> {
    using type = type_list<>;
};

template<template<class> class P, class T, class... Rest>
struct filter<P, type_list<T, Rest...>> {
private:
    using tail = typename filter<P, type_list<Rest...>>::type;
public:
    using type = std::conditional_t<P<T>::value, typename push_front<T, tail>::type, tail>;
};

template<template<class> class P, class List>
using filter_t = typename filter<P, List>::type;

template<class A, class B>
struct concat;

template<class... A, class... B>
struct concat<type_list<A...>, type_list<B...>> {
    using type = type_list<A..., B...>;
};

template<class A, class B>
using concat_t = typename concat<A, B>::type;

template<class T, class List>
struct contains;

template<class T>
struct contains<T, type_list<>> : std::false_type {};

template<class T, class Head, class... Tail>
struct contains<T, type_list<Head, Tail...>>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type, contains<T, type_list<Tail...>>> {};

template<class Seen, class Rest>
struct unique_impl;

template<class Seen>
struct unique_impl<Seen, type_list<>> {
    using type = Seen;
};

template<class... Seen, class T, class... Rest>
struct unique_impl<type_list<Seen...>, type_list<T, Rest...>> {
    using next_seen = std::conditional_t<contains<T, type_list<Seen...>>::value,
        type_list<Seen...>, type_list<Seen..., T>>;
    using type = typename unique_impl<next_seen, type_list<Rest...>>::type;
};

template<class List>
using unique_t = typename unique_impl<type_list<>, List>::type;

template<bool ChooseThen, class Then, class Else>
struct lazy_type {
    using type = typename Then::type;
};

template<bool ChooseThen, class Then, class Else>
using lazy_type_t = typename lazy_type<ChooseThen, Then, Else>::type;

template<class... Ts>
struct value_sig {};

template<class E>
struct error_sig {};

struct stopped_sig {};

template<class F, class Sig>
struct transform_one;

template<class F, class... Ts>
    requires std::is_invocable_v<F, Ts...>
struct transform_one<F, value_sig<Ts...>> {
private:
    using result = std::invoke_result_t<F, Ts...>;
public:
    using type = std::conditional_t<std::is_void_v<result>, value_sig<>, value_sig<result>>;
};

template<class F, class E>
struct transform_one<F, error_sig<E>> {
    using type = value_sig<int>;
};

template<class F>
struct transform_one<F, stopped_sig> {
    using type = value_sig<int>;
};

template<class F, class List>
struct transform_completion_signatures;

template<class F, class... Sigs>
    requires (requires { typename transform_one<F, Sigs>::type; } && ...)
struct transform_completion_signatures<F, type_list<Sigs...>> {
    using type = unique_t<type_list<typename transform_one<F, Sigs>::type...>>;
};

template<class F, class List>
using transform_completion_signatures_t = typename transform_completion_signatures<F, List>::type;

} // namespace c04
