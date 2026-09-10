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

constexpr int cmp(row const& item, std::string_view key) {
    for (std::size_t i = 0; i < row::key_width; ++i) {
        const char r = i < key.size() ? key[i] : '\0';
        if (item.key[i] != r) return item.key[i] < r ? -1 : 1;
        if (item.key[i] == '\0') return i == key.size() ? 0 : -1;
    }
    return key.size() + 1 == row::key_width ? 0 : -1;
}

constexpr int cmp(row const& a, row const& b) {
    for (std::size_t i = 0; i < row::key_width; ++i) {
        if (a.key[i] != b.key[i]) return a.key[i] < b.key[i] ? -1 : 1;
        if (a.key[i] == '\0') return 0;
    }
    return 0;
}

constexpr bool same_key(row const& a, row const& b) { return cmp(a, b) == 0; }
constexpr bool same_key(row const& a, std::string_view b) { return cmp(a, b) == 0; }

template<std::size_t N>
struct table {
    static constexpr std::size_t size = N;
    std::array<row, N> rows{};
};

template<std::size_t N>
consteval auto sort_by_key(std::array<row, N> input) {
    for (std::size_t pass = 0; pass < N; ++pass) {
        for (std::size_t i = 1; i < N; ++i) {
            if (cmp(input[i], input[i - 1]) < 0) {
                auto tmp = input[i - 1];
                input[i - 1] = input[i];
                input[i] = tmp;
            }
        }
    }
    return input;
}

template<std::size_t N>
consteval auto normalize_same_size(std::array<row, N> input) {
    return sort_by_key(input);
}

template<auto Raw>
consteval std::size_t unique_count() {
    std::size_t total = 0;
    for (std::size_t i = 0; i < Raw.size(); ++i) {
        bool first = true;
        for (std::size_t j = 0; j < i; ++j) first = first && !same_key(Raw[i], Raw[j]);
        total += first ? 1u : 0u;
    }
    return total;
}

template<auto Raw>
consteval auto make_table() {
    table<unique_count<Raw>()> out{};
    for (std::size_t i = 0, w = 0; i < Raw.size(); ++i) {
        bool first = true;
        for (std::size_t j = 0; j < i; ++j) first = first && !same_key(Raw[i], Raw[j]);
        if (first) out.rows[w++] = Raw[i];
    }
    out.rows = sort_by_key(out.rows);
    return out;
}

template<std::size_t N>
constexpr std::optional<int> find(table<N> const& input, std::string_view key) {
    std::size_t lo = 0;
    std::size_t hi = N;
    while (lo < hi) {
        const auto mid = lo + (hi - lo) / 2;
        const int order = cmp(input.rows[mid], key);
        if (order == 0) return input.rows[mid].value;
        if (order < 0) lo = mid + 1;
        else hi = mid;
    }
    return std::nullopt;
}
}
