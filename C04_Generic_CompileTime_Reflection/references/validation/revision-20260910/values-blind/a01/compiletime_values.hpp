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

constexpr int compare_chars(char left, char right) noexcept {
    const auto lhs = static_cast<unsigned char>(left);
    const auto rhs = static_cast<unsigned char>(right);
    if (lhs < rhs) return -1;
    if (lhs > rhs) return 1;
    return 0;
}

template<std::size_t N>
consteval row make_row(char const (&text)[N], int value) {
    static_assert(N <= row::key_width, "A01 keys are limited to 15 characters");
    row out{};
    for (std::size_t i = 0; i + 1 < N; ++i) {
        if (static_cast<unsigned char>(text[i]) > 0x7f) {
            throw "A01 keys must be ASCII";
        }
        out.key[i] = text[i];
    }
    out.value = value;
    return out;
}

constexpr int compare_key(row const& left, row const& right) noexcept {
    for (std::size_t i = 0; i < row::key_width; ++i) {
        const int cmp = compare_chars(left.key[i], right.key[i]);
        if (cmp != 0 || left.key[i] == '\0') return cmp;
    }
    return 0;
}

constexpr int compare_key(row const& left, std::string_view right) noexcept {
    for (std::size_t i = 0; i < row::key_width; ++i) {
        const char rhs = i < right.size() ? right[i] : '\0';
        const int cmp = compare_chars(left.key[i], rhs);
        if (cmp != 0 || left.key[i] == '\0') return cmp;
    }
    return right.size() <= row::key_width ? 0 : -1;
}

constexpr bool same_key(row const& left, row const& right) noexcept {
    return compare_key(left, right) == 0;
}

constexpr bool same_key(row const& left, std::string_view right) noexcept {
    return compare_key(left, right) == 0;
}

template<std::size_t N>
struct table {
    static constexpr std::size_t size = N;
    std::array<row, N> rows{};
};

template<std::size_t N>
consteval auto sort_by_key(std::array<row, N> input) {
    for (std::size_t i = 1; i < N; ++i) {
        row current = input[i];
        std::size_t j = i;
        while (j != 0 && compare_key(current, input[j - 1]) < 0) {
            input[j] = input[j - 1];
            --j;
        }
        input[j] = current;
    }
    return input;
}

template<auto Raw>
consteval std::size_t unique_count() {
    std::size_t count = 0;
    for (std::size_t i = 0; i < Raw.size(); ++i) {
        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            seen = seen || same_key(Raw[i], Raw[j]);
        }
        if (!seen) ++count;
    }
    return count;
}

template<std::size_t N>
consteval auto normalize_same_size(std::array<row, N> input) {
    return sort_by_key(input);
}

template<auto Raw>
consteval auto make_table() {
    table<unique_count<Raw>()> out{};
    std::size_t write = 0;
    for (std::size_t i = 0; i < Raw.size(); ++i) {
        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            seen = seen || same_key(Raw[i], Raw[j]);
        }
        if (!seen) {
            out.rows[write++] = Raw[i];
        }
    }
    out.rows = sort_by_key(out.rows);
    return out;
}

template<std::size_t N>
constexpr std::optional<int> find(table<N> const& input, std::string_view key) noexcept {
    std::size_t first = 0;
    std::size_t last = N;
    while (first != last) {
        const std::size_t mid = first + (last - first) / 2;
        const int cmp = compare_key(input.rows[mid], key);
        if (cmp < 0) {
            first = mid + 1;
        } else if (cmp > 0) {
            last = mid;
        } else {
            return input.rows[mid].value;
        }
    }
    return std::nullopt;
}
}
