#pragma once
#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <c05/types.hpp>
#include <c05/utf.hpp>

namespace c05 {
static_assert(CHAR_BIT == 8, "C05 wire format requires 8-bit bytes");

template<class T>
    requires std::unsigned_integral<T> && (!std::same_as<T, bool>)
inline std::expected<T, DataError> read_be(std::span<const std::byte> input, std::size_t& cursor)
{
    constexpr auto width = sizeof(T);
    if (cursor > input.size() || input.size() - cursor < width) {
        return std::unexpected(DataError{Errc::incomplete_input, cursor, OffsetUnit::byte, "integer"});
    }
    T value = 0;
    for (std::size_t i = 0; i != width; ++i) {
        value = static_cast<T>((value << 8) | static_cast<T>(std::to_integer<unsigned char>(input[cursor + i])));
    }
    cursor += width;
    return value;
}

template<class T>
    requires std::unsigned_integral<T> && (!std::same_as<T, bool>)
inline void append_be(std::vector<std::byte>& out, T value)
{
    for (std::size_t i = sizeof(T); i != 0; --i) {
        out.push_back(static_cast<std::byte>((value >> ((i - 1) * 8)) & std::numeric_limits<unsigned char>::max()));
    }
}

inline std::expected<std::span<const std::byte>, DataError> take_bytes(
    std::span<const std::byte> input, std::size_t& cursor, std::size_t count)
{
    if (cursor > input.size() || input.size() - cursor < count) {
        return std::unexpected(DataError{Errc::incomplete_input, cursor, OffsetUnit::byte, "bytes"});
    }
    auto result = input.subspan(cursor, count);
    cursor += count;
    return result;
}

inline std::expected<std::string, DataError> read_utf8_field(std::span<const std::byte> input, std::size_t& cursor)
{
    const auto original = cursor;
    auto probe = cursor;
    auto length = read_be<std::uint32_t>(input, probe);
    if (!length) {
        return std::unexpected(length.error());
    }
    if (*length > max_text_bytes) {
        return std::unexpected(DataError{Errc::limit_exceeded, original, OffsetUnit::byte, "utf8_field"});
    }
    const auto text_start = probe;
    auto bytes = take_bytes(input, probe, *length);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    std::string text;
    text.reserve(bytes->size());
    for (auto b : *bytes) {
        text.push_back(static_cast<char>(std::to_integer<unsigned char>(b)));
    }
    auto ok = validate_utf8(text);
    if (!ok) {
        auto err = ok.error();
        err.offset += text_start;
        err.field = "utf8_field";
        return std::unexpected(err);
    }
    cursor = probe;
    return text;
}
}
