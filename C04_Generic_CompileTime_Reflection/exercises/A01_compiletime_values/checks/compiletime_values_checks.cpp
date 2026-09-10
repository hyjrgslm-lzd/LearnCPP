#include <compiletime_values.hpp>

#include <check.hpp>

#include <array>
#include <iostream>

namespace {
using namespace c04_values;

inline constexpr auto raw_prices = std::array{
    make_row("mp", 30),
    make_row("hp", 100),
    make_row("atk", 7),
    make_row("mp", 99),
    make_row("def", 12),
};

inline constexpr auto empty_prices = std::array<row, 0>{};
inline constexpr auto boundary_prices = std::array{
    make_row("", 1),
    make_row("123456789012345", 15),
};
}

int main() {
    constexpr auto sorted = sort_by_key(raw_prices);
    check(sorted[0].value == 7 && same_key(sorted[0], "atk"), "sort orders literal keys");
    check(sorted[3].value == 30 && same_key(sorted[3], "mp"), "sort is stable for equal keys");

    constexpr auto table = make_table<raw_prices>();
    check(decltype(table)::size == 4, "duplicate keys shrink the result type");
    check(same_key(table.rows[0], "atk") && same_key(table.rows[1], "def") &&
          same_key(table.rows[2], "hp") && same_key(table.rows[3], "mp"),
        "deduped table is sorted by key");
    check(find(table, "mp").value_or(-1) == 30, "duplicate keys keep the first value before sorting");
    check(find(table, "hp").value_or(-1) == 100, "binary lookup finds existing literal key");
    check(!find(table, "agi").has_value(), "binary lookup rejects a missing key");

    constexpr auto empty = make_table<empty_prices>();
    check(decltype(empty)::size == 0, "empty input produces table<0>");
    check(!find(empty, "hp").has_value(), "empty table lookup is empty");

    constexpr auto boundary = make_table<boundary_prices>();
    check(decltype(boundary)::size == 2, "empty and 15-character ASCII keys are accepted");
    check(find(boundary, "").value_or(-1) == 1, "empty literal key is a valid key");
    check(find(boundary, "123456789012345").value_or(-1) == 15, "15-character ASCII key is accepted");
    check(!find(boundary, std::string_view{"", 1}).has_value(), "embedded NUL lookup is not the empty key");

    constexpr auto by_parameter = normalize_same_size(raw_prices);
    check(by_parameter.size() == raw_prices.size(), "ordinary consteval parameters keep the input type shape");

    std::cout << "A01 compile-time values contract passed\n";
}
