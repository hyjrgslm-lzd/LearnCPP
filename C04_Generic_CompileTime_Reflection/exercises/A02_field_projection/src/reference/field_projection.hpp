#pragma once
#include <projection_schema.hpp>

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace c04_projection {
namespace reference_detail {
template<class T>
concept has_schema = requires { schema<std::remove_cvref_t<T>>::fields; };

template<fixed_string Left, fixed_string Right>
consteval bool equal() {
    if constexpr (Left.size() != Right.size()) return false;
    else {
        for (std::size_t i = 0; i < Left.size(); ++i) {
            if (Left.value[i] != Right.value[i]) return false;
        }
        return true;
    }
}

template<fixed_string Text>
consteval bool ascii_identifier() {
    if constexpr (Text.size() == 0) return false;
    else {
        auto ok_first = [](char ch) consteval {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_';
        };
        auto ok_next = [](char ch) consteval {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                   (ch >= '0' && ch <= '9') || ch == '_';
        };
        if (!ok_first(Text.value[0])) return false;
        for (std::size_t i = 1; i < Text.size(); ++i) if (!ok_next(Text.value[i])) return false;
        return true;
    }
}

template<fixed_string Name, class Record, class Fields, std::size_t... I>
consteval std::size_t find_field(std::index_sequence<I...>) {
    std::size_t found = static_cast<std::size_t>(-1);
    ((equal<Name, std::tuple_element_t<I, Fields>::name>() ? found = I : 0), ...);
    return found;
}

template<fixed_string Name, class Record>
consteval std::size_t find_field() {
    using fields = decltype(schema<Record>::fields);
    return find_field<Name, Record, fields>(std::make_index_sequence<std::tuple_size_v<fields>>{});
}

template<fixed_string Text, std::size_t Begin, std::size_t Count>
consteval auto substring() {
    char out[Count + 1]{};
    for (std::size_t i = 0; i < Count; ++i) out[i] = Text.value[Begin + i];
    return fixed_string<Count + 1>{out};
}

template<fixed_string... Names>
struct names {};

template<class List, fixed_string Name>
struct push_name;

template<fixed_string... Names, fixed_string Name>
struct push_name<names<Names...>, Name> {
    using type = names<Names..., Name>;
};

template<fixed_string Text, std::size_t Pos>
consteval std::size_t next_separator() {
    for (std::size_t i = Pos; i < Text.size(); ++i) if (Text.value[i] == ',') return i;
    return Text.size();
}

template<fixed_string Text, std::size_t Pos, class Out, bool End = (Pos == Text.size())>
struct parse_projection;

template<fixed_string Text, std::size_t Pos, fixed_string... Names>
struct parse_projection<Text, Pos, names<Names...>, true> {
    using type = names<Names...>;
};

template<fixed_string Text, std::size_t Pos, fixed_string... Names>
struct parse_projection<Text, Pos, names<Names...>, false> {
    static constexpr std::size_t sep = next_separator<Text, Pos>();
    static constexpr std::size_t count = sep - Pos;
    static_assert(count != 0, "projection component must not be empty");
    static constexpr auto name = substring<Text, Pos, count>();
    static_assert(ascii_identifier<name>(), "projection field name must be an ASCII identifier");
    using with_name = typename push_name<names<Names...>, name>::type;
    using type = typename parse_projection<Text, (sep == Text.size() ? sep : sep + 1), with_name>::type;
};

template<fixed_string... Names>
struct all_unique : std::true_type {};

template<fixed_string Name, fixed_string... Rest>
struct all_unique<Name, Rest...> : std::bool_constant<((!equal<Name, Rest>()) && ...) && all_unique<Rest...>::value> {};

template<class Record, fixed_string Name>
consteval void require_known() {
    static_assert(find_field<Name, Record>() != static_cast<std::size_t>(-1), "unknown projection field");
}

template<fixed_string Name, class Object>
constexpr decltype(auto) member(Object&& object) {
    using record = std::remove_cvref_t<Object>;
    static_assert(has_schema<record>, "unknown projection record");
    static_assert(ascii_identifier<Name>(), "projection field name must be an ASCII identifier");
    constexpr auto index = find_field<Name, record>();
    static_assert(index != static_cast<std::size_t>(-1), "unknown projection field");
    return std::forward<Object>(object).*std::get<index>(schema<record>::fields).member;
}

template<class Record, fixed_string... Names>
consteval void validate(names<Names...>) {
    static_assert(all_unique<Names...>::value, "duplicate projection field");
    (require_known<Record, Names>(), ...);
}

template<class T, fixed_string... Names>
constexpr auto project_impl(T& object, names<Names...> list) {
    validate<std::remove_cvref_t<T>>(list);
    return std::tuple<decltype(member<Names>(object))...>(member<Names>(object)...);
}
}

template<fixed_string Name, class T>
constexpr decltype(auto) get(T&& object) {
    return reference_detail::member<Name>(std::forward<T>(object));
}

template<fixed_string Spec, class T>
    requires std::is_lvalue_reference_v<T&&>
constexpr auto project(T&& object) {
    static_assert(Spec.size() == 0 || Spec.value[Spec.size() - 1] != ',',
        "projection component must not be empty");
    using parsed = typename reference_detail::parse_projection<Spec, 0, reference_detail::names<>>::type;
    return reference_detail::project_impl(object, parsed{});
}

template<fixed_string Spec, class T>
    requires (!std::is_lvalue_reference_v<T&&>)
constexpr auto project(T&&) = delete;
}
