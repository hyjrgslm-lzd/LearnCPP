#pragma once

#include <projection_schema.hpp>

#include <boost/mp11.hpp>
#include <boost/mp11/map.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
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

template<class T>
struct member_pointer_traits;

template<class Owner, class Value>
struct member_pointer_traits<Value Owner::*> {
    using owner_type = Owner;
    using value_type = Value;
};

template<class Pair>
using field_key_t = mp11::mp_first<Pair>;

template<class Pair>
using field_member_constant_t = mp11::mp_second<Pair>;

template<class Pair>
inline constexpr auto field_member_v = field_member_constant_t<Pair>::value;

template<class Pair>
using field_value_type_t = typename member_pointer_traits<std::remove_cv_t<decltype(field_member_v<Pair>)>>::value_type;

template<class Key>
inline constexpr std::string_view field_name_v{Key::name.value, Key::name.size()};

template<class Pair>
inline constexpr std::string_view field_value_label_v = [] {
    using value = field_value_type_t<Pair>;
    if constexpr (std::is_same_v<value, int>) {
        return std::string_view{"int"};
    } else if constexpr (std::is_same_v<value, bool>) {
        return std::string_view{"bool"};
    } else if constexpr (std::is_same_v<value, std::string>) {
        return std::string_view{"string"};
    } else {
        return std::string_view{"unknown"};
    }
}();

template<class Field>
struct to_pair;

template<fixed_string Name, auto Member>
struct to_pair<c04_projection::field<Name, Member>> {
    using type = mp11::mp_list<key<Name>, member_c<Member>>;
};

template<class Tuple>
struct tuple_to_map;

template<class... Fields>
struct tuple_to_map<std::tuple<Fields...>> {
    using type = mp11::mp_list<typename to_pair<Fields>::type...>;
};

template<class Record>
using schema_map_t = typename tuple_to_map<std::remove_cv_t<decltype(c04_projection::schema<Record>::fields)>>::type;

template<class Map, class Key>
struct required_pair {
    using found = mp11::mp_map_find<Map, Key>;
    static_assert(!std::is_same_v<found, void>, "mp11 map key not found");
    using type = found;
};

template<class Map, class Key>
using required_pair_t = typename required_pair<Map, Key>::type;

template<class Record, fixed_string Name>
using find_field_t = mp11::mp_map_find<schema_map_t<Record>, key<Name>>;

template<class Record, fixed_string Name>
using required_field_t = required_pair_t<schema_map_t<Record>, key<Name>>;

template<class Record, fixed_string Name>
inline constexpr bool has_field_v = !std::is_same_v<find_field_t<Record, Name>, void>;

template<class Record>
inline constexpr std::size_t field_count_v = mp11::mp_size<schema_map_t<Record>>::value;

template<class Record>
inline constexpr bool keys_are_unique_v =
    mp11::mp_size<schema_map_t<Record>>::value ==
    mp11::mp_size<mp11::mp_unique<mp11::mp_transform<field_key_t, schema_map_t<Record>>>>::value;

template<class Map, class Pair>
using insert_field_t = mp11::mp_map_insert<Map, Pair>;

template<class Map, class Pair>
using replace_field_t = mp11::mp_map_replace<Map, Pair>;

template<class Map, class Pair, template<class...> class F>
using update_field_t = mp11::mp_map_update<Map, Pair, F>;

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
decltype(auto) get_by_key(Object&& object) {
    using record = std::remove_cvref_t<Object>;
    using pair = required_field_t<record, Name>;
    return std::forward<Object>(object).*field_member_v<pair>;
}

template<class Record, class Visitor>
decltype(auto) visit_field_type(std::size_t index, Visitor&& visitor) {
    using map = schema_map_t<Record>;
    if (index >= field_count_v<Record>) {
        throw std::out_of_range{"field index out of range"};
    }
    return mp11::mp_with_index<field_count_v<Record>>(index, [&]<class I>(I) -> decltype(auto) {
        using pair = mp11::mp_at_c<map, I::value>;
        return std::forward<Visitor>(visitor).template operator()<pair>();
    });
}
}
