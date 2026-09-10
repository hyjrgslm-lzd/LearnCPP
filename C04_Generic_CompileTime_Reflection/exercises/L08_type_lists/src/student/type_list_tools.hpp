#pragma once

namespace c04 {

template<class... Ts>
struct type_list {};

template<template<class> class F, class List>
using map_t = type_list<>;

template<template<class> class P, class List>
using filter_t = type_list<>;

template<class A, class B>
using concat_t = type_list<>;

template<class List>
using unique_t = List;

template<bool ChooseThen, class Then, class Else>
struct lazy_type {
    using type = void;
};

template<bool ChooseThen, class Then, class Else>
using lazy_type_t = typename lazy_type<ChooseThen, Then, Else>::type;

template<class... Ts>
struct value_sig {};

template<class E>
struct error_sig {};

struct stopped_sig {};

template<class F, class List>
struct transform_completion_signatures {};

template<class F, class... Sigs>
struct transform_completion_signatures<F, type_list<Sigs...>> {
    using type = type_list<>;
};

template<class F, class List>
using transform_completion_signatures_t = typename transform_completion_signatures<F, List>::type;

} // namespace c04
