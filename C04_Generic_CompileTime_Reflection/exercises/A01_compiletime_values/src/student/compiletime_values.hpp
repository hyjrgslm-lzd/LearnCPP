#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace c04_values {
struct row {
    static constexpr std::size_t key_width = 16;
    std::array<char, key_width> key{};
    int value{};
    friend constexpr bool operator==(row const&, row const&) = default;
};

template<std::size_t N>
consteval row make_row(char const (&text)[N], int value) {
    static_assert(N <= row::key_width, "literal key is too long");
    row out{};
    for (std::size_t i = 0; i + 1 < N; ++i) {
        const auto ch = static_cast<unsigned char>(text[i]);
        if (ch == 0 || ch > 0x7F) throw "literal key must contain only non-NUL ASCII bytes";
        out.key[i] = text[i];
    }
    out.value = value;
    return out;
}

constexpr bool same_key(row const& left, std::string_view right) {
    for (std::size_t i = 0; i < row::key_width; ++i) {
        const char l = left.key[i];
        const char r = i < right.size() ? right[i] : '\0';
        if (l != r) return false;
        if (l == '\0') return i == right.size();
    }
    return right.size() + 1 == row::key_width;
}

template<std::size_t N>
struct table {
    static constexpr std::size_t size = N;
    std::array<row, N> rows{};
};

template<std::size_t N>
consteval auto normalize_same_size(std::array<row, N> input) {
    return input;
}

template<std::size_t N>
consteval auto sort_by_key(std::array<row, N> input) {
    return input;
}

template<auto Raw>
consteval auto make_table() {
    return table<Raw.size()>{Raw};
}

template<std::size_t N>
constexpr std::optional<int> find(table<N> const& input, std::string_view key) {
    for (auto const& item : input.rows) {
        if (same_key(item, key)) return item.value;
    }
    return std::nullopt;
}
}
