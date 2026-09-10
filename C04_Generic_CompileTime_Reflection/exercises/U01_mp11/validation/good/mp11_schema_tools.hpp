#pragma once
#include <projection_schema.hpp>

#include <boost/mp11.hpp>
#include <boost/mp11/algorithm.hpp>
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

template<auto Pointer>
using member_c = std::integral_constant<decltype(Pointer), Pointer>;

template<class Pair>
using field_key_t = mp11::mp_first<Pair>;

template<class Pair>
using field_member_t = mp11::mp_second<Pair>;

template<class Key>
inline constexpr std::string_view field_name_v{Key::name.value, Key::name.size()};

template<auto Pointer>
struct pointer_info;

template<class Owner, class Value, Value Owner::* Pointer>
struct pointer_info<Pointer> {
    using value_type = Value;
};

template<class Pair>
using field_value_type_t = typename pointer_info<field_member_t<Pair>::value>::value_type;

template<class Pair>
inline constexpr std::string_view field_value_label_v =
    std::is_same_v<field_value_type_t<Pair>, int> ? "int" :
    std::is_same_v<field_value_type_t<Pair>, bool> ? "bool" :
    std::is_same_v<field_value_type_t<Pair>, std::string> ? "string" : "unknown";

namespace detail {
template<class Field>
struct schema_entry;

template<fixed_string Name, auto Pointer>
struct schema_entry<c04_projection::field<Name, Pointer>> {
    using type = mp11::mp_list<key<Name>, member_c<Pointer>>;
};

template<class Tuple>
struct tuple_entries;

template<class... Fields>
struct tuple_entries<std::tuple<Fields...>> {
    using type = mp11::mp_list<typename schema_entry<Fields>::type...>;
};
}

template<class Record>
using schema_map_t = typename detail::tuple_entries<
    std::remove_cv_t<decltype(c04_projection::schema<std::remove_cvref_t<Record>>::fields)>>::type;

template<class Record, fixed_string Name>
using find_field_t = mp11::mp_map_find<schema_map_t<Record>, key<Name>>;

template<class Map, class Key>
using required_pair_t = mp11::mp_map_find<Map, Key>;

template<class Record, fixed_string Name>
inline constexpr bool has_field_v = !std::is_same_v<find_field_t<Record, Name>, void>;

template<class Record, fixed_string Name>
struct required_field {
    using type = find_field_t<Record, Name>;
    static_assert(!std::is_same_v<type, void>, "mp11 map key not found");
};

template<class Record, fixed_string Name>
using required_field_t = typename required_field<Record, Name>::type;

template<class Record>
inline constexpr std::size_t field_count_v = mp11::mp_size<schema_map_t<Record>>::value;

template<class Record>
inline constexpr bool keys_are_unique_v = std::is_same_v<
    mp11::mp_map_keys<schema_map_t<Record>>,
    mp11::mp_unique<mp11::mp_map_keys<schema_map_t<Record>>>>;

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
    using pair = required_field_t<std::remove_cvref_t<Object>, Name>;
    return std::forward<Object>(object).*field_member_t<pair>::value;
}

template<class Record, class Visitor>
decltype(auto) visit_field_type(std::size_t index, Visitor&& visitor) {
    using map = schema_map_t<Record>;
    constexpr std::size_t count = mp11::mp_size<map>::value;
    if (index >= count) {
        throw std::out_of_range("field index out of range");
    }
    if constexpr (count > 0) {
        return mp11::mp_with_index<count>(index, [&]<class I>(I) -> decltype(auto) {
            using pair = mp11::mp_at_c<map, I::value>;
            return std::forward<Visitor>(visitor).template operator()<pair>();
        });
    }
}
}
