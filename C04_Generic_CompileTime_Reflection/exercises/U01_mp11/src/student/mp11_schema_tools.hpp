#pragma once
#include <projection_schema.hpp>

#include <boost/mp11.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/map.hpp>

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace c04_mp11 {
namespace mp11 = boost::mp11;

using c04_projection::fixed_string;

template<fixed_string Name>
struct key {
    static constexpr auto name = Name;
};

template<auto Member>
using member_c = std::integral_constant<decltype(Member), Member>;

template<class Pair>
using field_key_t = mp11::mp_first<Pair>;

template<class Pair>
using field_member_t = mp11::mp_second<Pair>;

template<class Key>
inline constexpr std::string_view field_name_v{Key::name.value, Key::name.size()};

template<class Pair>
using field_value_type_t = void;

template<class Pair>
inline constexpr std::string_view field_value_label_v = "unknown";

template<class Record>
using schema_map_t = mp11::mp_list<>;

template<class Record, fixed_string Name>
using find_field_t = mp11::mp_map_find<schema_map_t<Record>, key<Name>>;

template<class Map, class Key>
using required_pair_t = mp11::mp_map_find<Map, Key>;

template<class Record, fixed_string Name>
inline constexpr bool has_field_v = !std::is_same_v<find_field_t<Record, Name>, void>;

template<class Record, fixed_string Name>
using required_field_t = find_field_t<Record, Name>;

template<class Record>
inline constexpr std::size_t field_count_v = mp11::mp_size<schema_map_t<Record>>::value;

template<class Record>
inline constexpr bool keys_are_unique_v = true;

template<class Map, class Entry>
using insert_field_t = mp11::mp_map_insert<Map, Entry>;

template<class Map, class Entry>
using replace_field_t = mp11::mp_map_replace<Map, Entry>;

template<class Map, class Entry, template<class...> class F>
using update_field_t = mp11::mp_map_update<Map, Entry, F>;

template<template<class...> class F, class Map>
using transform_fields_t = mp11::mp_transform<F, Map>;

template<template<class...> class P, class Map>
using filter_fields_t = mp11::mp_filter<P, Map>;

template<class List>
using unique_types_t = mp11::mp_unique<List>;

template<template<class...> class F, class... Lists>
using product_t = mp11::mp_product<F, Lists...>;

template<bool ChooseThen, class ThenProvider, class ElseProvider>
struct lazy_provider;

template<class ThenProvider, class ElseProvider>
struct lazy_provider<true, ThenProvider, ElseProvider> {
    using type = typename ThenProvider::type;
};

template<class ThenProvider, class ElseProvider>
struct lazy_provider<false, ThenProvider, ElseProvider> {
    using type = typename ElseProvider::type;
};

template<bool ChooseThen, class ThenProvider, class ElseProvider>
using lazy_provider_t = typename lazy_provider<ChooseThen, ThenProvider, ElseProvider>::type;

template<fixed_string Name, class Object>
constexpr decltype(auto) get_by_key(Object&& object) {
    return std::forward<Object>(object);
}

template<class Record, class Visitor>
decltype(auto) visit_field_type(std::size_t, Visitor&&) {
    throw std::out_of_range("field index out of range");
}
}
