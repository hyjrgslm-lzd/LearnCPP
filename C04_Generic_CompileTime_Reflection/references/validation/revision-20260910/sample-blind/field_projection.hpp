#pragma once

#include <projection_schema.hpp>

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace c04_projection {
namespace detail {
struct component {
    std::size_t begin{};
    std::size_t end{};
};

template<class T>
concept described_record = requires {
    schema<std::remove_cvref_t<T>>::fields;
};

template<fixed_string Text>
consteval bool is_empty() {
    return Text.size() == 0;
}

consteval bool is_ident_start(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_';
}

consteval bool is_ident_continue(char ch) {
    return is_ident_start(ch) || (ch >= '0' && ch <= '9');
}

template<fixed_string Text>
consteval bool valid_spec() {
    if constexpr (is_empty<Text>()) {
        return true;
    }

    bool need_start = true;
    for (std::size_t i = 0; i < Text.size(); ++i) {
        const char ch = Text.value[i];
        if (ch == ',') {
            if (need_start) {
                return false;
            }
            need_start = true;
        } else if (need_start) {
            if (!is_ident_start(ch)) {
                return false;
            }
            need_start = false;
        } else if (!is_ident_continue(ch)) {
            return false;
        }
    }
    return !need_start;
}

template<fixed_string Text>
consteval std::size_t component_count() {
    if constexpr (is_empty<Text>()) {
        return 0;
    }

    std::size_t count = 1;
    for (std::size_t i = 0; i < Text.size(); ++i) {
        if (Text.value[i] == ',') {
            ++count;
        }
    }
    return count;
}

template<fixed_string Text>
consteval auto components() {
    constexpr auto count = component_count<Text>();
    std::array<component, count> result{};
    std::size_t part = 0;
    std::size_t start = 0;

    for (std::size_t i = 0; i <= Text.size(); ++i) {
        if (i == Text.size() || Text.value[i] == ',') {
            if constexpr (count > 0) {
                result[part++] = {start, i};
            }
            start = i + 1;
        }
    }
    return result;
}

template<fixed_string Left, fixed_string Right>
consteval bool same_text() {
    if constexpr (Left.size() != Right.size()) {
        return false;
    }

    for (std::size_t i = 0; i < Left.size(); ++i) {
        if (Left.value[i] != Right.value[i]) {
            return false;
        }
    }
    return true;
}

template<fixed_string Text, fixed_string Name>
consteval bool same_slice(component part) {
    if (part.end - part.begin != Name.size()) {
        return false;
    }

    for (std::size_t i = 0; i < Name.size(); ++i) {
        if (Text.value[part.begin + i] != Name.value[i]) {
            return false;
        }
    }
    return true;
}

template<fixed_string Text>
consteval bool no_duplicate_components() {
    if constexpr (!valid_spec<Text>()) {
        return true;
    }

    constexpr auto parts = components<Text>();
    for (std::size_t i = 0; i < parts.size(); ++i) {
        for (std::size_t j = i + 1; j < parts.size(); ++j) {
            if (parts[i].end - parts[i].begin != parts[j].end - parts[j].begin) {
                continue;
            }

            bool equal = true;
            for (std::size_t k = 0; k < parts[i].end - parts[i].begin; ++k) {
                if (Text.value[parts[i].begin + k] != Text.value[parts[j].begin + k]) {
                    equal = false;
                    break;
                }
            }
            if (equal) {
                return false;
            }
        }
    }
    return true;
}

template<fixed_string Name, class Record, std::size_t Index = 0>
consteval std::size_t field_index() {
    static_assert(described_record<Record>, "get requires a schema for the record type");
    constexpr auto count = std::tuple_size_v<decltype(schema<std::remove_cvref_t<Record>>::fields)>;

    if constexpr (Index >= count) {
        static_assert(Index < count, "unknown field name");
        return 0;
    } else {
        using field_type = std::tuple_element_t<Index, decltype(schema<std::remove_cvref_t<Record>>::fields)>;
        if constexpr (same_text<Name, field_type::name>()) {
            return Index;
        } else {
            return field_index<Name, Record, Index + 1>();
        }
    }
}

template<fixed_string Text, class Record, std::size_t Part, std::size_t Index = 0>
consteval std::size_t field_index_for_component() {
    static_assert(described_record<Record>, "project requires a schema for the record type");
    constexpr auto count = std::tuple_size_v<decltype(schema<std::remove_cvref_t<Record>>::fields)>;

    if constexpr (Index >= count) {
        static_assert(Index < count, "unknown field in projection spec");
        return 0;
    } else {
        using field_type = std::tuple_element_t<Index, decltype(schema<std::remove_cvref_t<Record>>::fields)>;
        constexpr auto parts = components<Text>();
        if constexpr (same_slice<Text, field_type::name>(parts[Part])) {
            return Index;
        } else {
            return field_index_for_component<Text, Record, Part, Index + 1>();
        }
    }
}

template<fixed_string Text, class T, std::size_t... Parts>
constexpr auto project_impl(T&& object, std::index_sequence<Parts...>) {
    using record_type = std::remove_cvref_t<T>;
    return std::forward_as_tuple(
        (object.*std::tuple_element_t<
            field_index_for_component<Text, record_type, Parts>(),
            decltype(schema<record_type>::fields)>::member)...);
}
}

template<fixed_string Name, class T>
constexpr decltype(auto) get(T&& object) {
    using record_type = std::remove_cvref_t<T>;
    constexpr auto index = detail::field_index<Name, record_type>();
    using field_type = std::tuple_element_t<index, decltype(schema<record_type>::fields)>;
    return (std::forward<T>(object).*field_type::member);
}

template<fixed_string Spec, class T>
    requires std::is_lvalue_reference_v<T&&>
constexpr auto project(T&& object) {
    static_assert(detail::valid_spec<Spec>(),
        "projection spec must be empty or comma-separated ASCII identifiers");
    static_assert(detail::no_duplicate_components<Spec>(),
        "projection spec contains a duplicate field");

    if constexpr (Spec.size() == 0) {
        return std::tuple<>{};
    } else {
        return detail::project_impl<Spec>(
            std::forward<T>(object),
            std::make_index_sequence<detail::component_count<Spec>()>{});
    }
}

template<fixed_string Spec, class T>
    requires (!std::is_lvalue_reference_v<T&&>)
constexpr auto project(T&&) = delete;
}
