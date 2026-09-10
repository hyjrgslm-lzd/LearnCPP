#pragma once
#include <projection_schema.hpp>

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace c04_projection {
namespace good_detail {
template<class T>
concept has_schema = requires { schema<std::remove_cvref_t<T>>::fields; };

template<auto Pointer>
struct member_pointer;

template<class Owner, class Member, Member Owner::* Pointer>
struct member_pointer<Pointer> {
    using owner_type = Owner;
    using member_type = Member;
};

template<fixed_string Text>
consteval bool identifier() {
    if constexpr (Text.size() == 0) return false;
    else {
        const auto first = Text.value[0];
        if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_')) return false;
        for (std::size_t i = 1; i < Text.size(); ++i) {
            const auto ch = Text.value[i];
            if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                  (ch >= '0' && ch <= '9') || ch == '_')) return false;
        }
        return true;
    }
}

template<fixed_string A, fixed_string B>
consteval bool same_name() {
    if constexpr (A.size() != B.size()) return false;
    else {
        for (std::size_t i = 0; i < A.size(); ++i) if (A.value[i] != B.value[i]) return false;
        return true;
    }
}

template<fixed_string Name, class Record, std::size_t... I>
consteval std::size_t field_index(std::index_sequence<I...>) {
    constexpr auto fields = schema<Record>::fields;
    std::size_t result = static_cast<std::size_t>(-1);
    ((same_name<Name, std::tuple_element_t<I, decltype(fields)>::name>() ? result = I : 0), ...);
    return result;
}

template<fixed_string Name, class Record>
consteval std::size_t field_index() {
    using fields_t = decltype(schema<Record>::fields);
    return field_index<Name, Record>(std::make_index_sequence<std::tuple_size_v<fields_t>>{});
}

template<std::size_t N>
struct literal_holder {
    fixed_string<N> text;
};

template<fixed_string Text, std::size_t Begin, std::size_t Len>
consteval auto slice() {
    char out[Len + 1]{};
    for (std::size_t i = 0; i < Len; ++i) out[i] = Text.value[Begin + i];
    return fixed_string<Len + 1>{out};
}

template<fixed_string... Names>
struct name_list {};

template<class List, fixed_string Name>
struct append;

template<fixed_string... Names, fixed_string Name>
struct append<name_list<Names...>, Name> {
    using type = name_list<Names..., Name>;
};

template<fixed_string Text, std::size_t Pos>
consteval std::size_t comma() {
    for (std::size_t i = Pos; i < Text.size(); ++i) if (Text.value[i] == ',') return i;
    return Text.size();
}

template<fixed_string Text, std::size_t Pos, class Acc, bool Done = (Pos == Text.size())>
struct parse;

template<fixed_string Text, std::size_t Pos, fixed_string... Names>
struct parse<Text, Pos, name_list<Names...>, true> {
    using type = name_list<Names...>;
};

template<fixed_string Text, std::size_t Pos, fixed_string... Names>
struct parse<Text, Pos, name_list<Names...>, false> {
    static constexpr std::size_t end = comma<Text, Pos>();
    static constexpr std::size_t len = end - Pos;
    static_assert(len != 0, "projection component must not be empty");
    static constexpr auto part = slice<Text, Pos, len>();
    static_assert(identifier<part>(), "projection field name must be an ASCII identifier");
    using next = typename append<name_list<Names...>, part>::type;
    using type = typename parse<Text, (end == Text.size() ? end : end + 1), next>::type;
};

template<fixed_string... Names>
struct all_unique : std::true_type {};

template<fixed_string Name, fixed_string... Rest>
struct all_unique<Name, Rest...> : std::bool_constant<((!same_name<Name, Rest>()) && ...) && all_unique<Rest...>::value> {};

template<class Record, fixed_string Name>
consteval void require_known() {
    static_assert(field_index<Name, Record>() != static_cast<std::size_t>(-1), "unknown projection field");
}

template<class Record, fixed_string... Names>
consteval void validate(name_list<Names...>) {
    static_assert(all_unique<Names...>::value, "duplicate projection field");
    (require_known<Record, Names>(), ...);
}

template<class Object, fixed_string Name>
constexpr decltype(auto) get_one(Object&& object) {
    using record = std::remove_cvref_t<Object>;
    static_assert(has_schema<record>, "unknown projection record");
    static_assert(identifier<Name>(), "projection field name must be an ASCII identifier");
    constexpr std::size_t index = field_index<Name, record>();
    static_assert(index != static_cast<std::size_t>(-1), "unknown projection field");
    constexpr auto descriptor = std::get<index>(schema<record>::fields);
    return std::forward<Object>(object).*descriptor.member;
}

template<class T, fixed_string... Names>
constexpr auto project_all(T& object, name_list<Names...>) {
    validate<std::remove_cvref_t<T>>(name_list<Names...>{});
    return std::tuple<decltype(get_one<T&, Names>(object))...>(get_one<T&, Names>(object)...);
}
}

template<fixed_string Name, class T>
constexpr decltype(auto) get(T&& object) {
    return good_detail::get_one<T, Name>(std::forward<T>(object));
}

template<fixed_string Spec, class T>
    requires std::is_lvalue_reference_v<T&&>
constexpr auto project(T&& object) {
    static_assert(Spec.size() == 0 || Spec.value[Spec.size() - 1] != ',',
        "projection component must not be empty");
    using names = typename good_detail::parse<Spec, 0, good_detail::name_list<>>::type;
    return good_detail::project_all(object, names{});
}

template<fixed_string Spec, class T>
    requires (!std::is_lvalue_reference_v<T&&>)
constexpr auto project(T&&) = delete;
}
