#pragma once
#include <check.hpp>
#include <record_schema.hpp>
#include <algorithm>
#include <concepts>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c04_record::checks {
struct AllFields {
    template<class T> void operator()(std::string_view, T&&) & noexcept {}
    template<class T> void operator()(std::string_view, T&&) && = delete;
};
struct OnlyInts { void operator()(std::string_view, int&) const noexcept {} };
template<class Backend, class T>
concept encodable = requires(const T& value) { Backend::encode_fields(value); };
template<class Backend, class T, class F>
concept visitable = requires(T&& value, F&& function) { Backend::visit_fields(std::forward<T>(value), std::forward<F>(function)); };

template<class Backend>
void run() {
    const Person original{7, true, "Ada"};
    const encoded_fields expected{{"id", "7"}, {"active", "true"}, {"name", "Ada"}};
    auto wire = Backend::encode_fields(original);
    check(wire == expected, "encoded fields cover the complete schema in declaration order");
    check(!encodable<Backend, Unsupported>, "unsupported field types are constrained out");
    check(!encodable<Backend, DuplicateNames>, "duplicate external names are constrained out");
    check(!encodable<Backend, NoSchema>, "records outside the declared support domain are rejected");
    check(!visitable<Backend, Person&, OnlyInts>, "callback must accept every field expression");
    check(noexcept(Backend::visit_fields(std::declval<Person&>(), AllFields{})), "noexcept follows every callback expression");

    auto decoded = Backend::template decode_fields<Person>(wire);
    check(decoded && *decoded == original, "all supported fields round trip");
    std::reverse(wire.begin(), wire.end());
    decoded = Backend::template decode_fields<Person>(wire);
    check(decoded && *decoded == original, "input field order is independent of declaration order");
    wire.back().value = "8";
    check(original.id == 7, "encoded fields own their data");
    check(Backend::format_record(original) == "record(id=7, active=true, name=\"Ada\")", "format output has specified field order and quoting");
    check(Backend::encode_fields(Renamed{12}) == encoded_fields{{"id", "12"}}, "external field name may differ from member spelling");
    check(Backend::template decode_fields<Renamed>(encoded_fields{{"id", "12"}}) == std::expected<Renamed, field_error>{Renamed{12}}, "renamed field decodes with its external name");

    const auto expect_error = [&](encoded_fields entries, field_error error) {
        auto result = Backend::template decode_fields<Person>(entries);
        check(!result && result.error() == error, "malformed field list rejects the actual input");
    };
    auto malformed = expected; malformed.push_back({"extra", "x"}); expect_error(malformed, field_error::unknown_field);
    malformed = expected; malformed.push_back(expected.front()); expect_error(malformed, field_error::duplicate_field);
    malformed = expected; malformed.erase(malformed.begin()); expect_error(malformed, field_error::missing_field);
    for (const auto& invalid : {"", "7x", "+7", " 7", "99999999999999999999999999999"}) {
        malformed = expected; malformed[0].value = invalid; expect_error(malformed, field_error::invalid_value);
    }
    malformed = expected; malformed[1].value = "1"; expect_error(malformed, field_error::invalid_value);
    for (int id : {0, -1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        Person value{id, false, std::string{"a\0b\n\"\\", 6}};
        auto result = Backend::template decode_fields<Person>(Backend::encode_fields(value));
        check(result && *result == value, "integer bounds and opaque string bytes round trip");
    }
    check(Backend::encode_fields(Empty{}).empty(), "empty record encodes no fields");
    check(Backend::template decode_fields<Empty>(encoded_fields{}).has_value(), "empty record decodes empty input");
    const auto empty_error = Backend::template decode_fields<Empty>(encoded_fields{{"x", "1"}});
    check(!empty_error && empty_error.error() == field_error::unknown_field, "empty schema rejects an unknown field");
    int empty_calls = 0;
    Backend::visit_fields(Empty{}, [&](std::string_view, auto&&) { ++empty_calls; });
    check(empty_calls == 0, "empty traversal never calls the callback");

    Person mutable_value = original;
    std::vector<std::string> order;
    Backend::visit_fields(mutable_value, [&](std::string_view name, auto&& member) {
        order.emplace_back(name);
        check(std::is_lvalue_reference_v<decltype(member)>, "lvalue object yields lvalue fields");
        if constexpr (std::same_as<std::remove_cvref_t<decltype(member)>, int>) {
            check(&member == &mutable_value.id, "visitation preserves member identity"); ++member;
        } else if constexpr (std::same_as<std::remove_cvref_t<decltype(member)>, bool>) member = !member;
        else member += "!";
    });
    check(order == std::vector<std::string>{"id", "active", "name"}, "visitation uses declaration order");
    check(mutable_value == Person{8, false, "Ada!"}, "callback writes through original fields");
    Backend::visit_fields(original, [&](std::string_view, auto&& member) {
        check(std::is_const_v<std::remove_reference_t<decltype(member)>>, "const object yields const fields");
    });
    std::string moved;
    Backend::visit_fields(std::move(mutable_value), [&](std::string_view, auto&& member) {
        check(std::is_rvalue_reference_v<decltype(member)>, "rvalue object yields rvalue fields");
        if constexpr (std::same_as<std::remove_cvref_t<decltype(member)>, std::string>) moved = std::move(member);
    });
    check(moved == "Ada!", "rvalue visitor can move a field without copying the record");
    Backend::visit_fields(mutable_value, AllFields{});
    int calls = 0;
    bool caught = false;
    try {
        Backend::visit_fields(mutable_value, [&](std::string_view, auto&&) {
            if (++calls == 2) throw std::runtime_error("visitor failure");
        });
    } catch (const std::runtime_error&) { caught = true; }
    check(caught && calls == 2, "throwing callback stops later fields without rolling back side effects");
}
}
