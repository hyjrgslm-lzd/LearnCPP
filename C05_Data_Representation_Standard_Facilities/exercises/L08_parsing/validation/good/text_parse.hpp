#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include <c05/types.hpp>

namespace c05_ex {
inline c05::DataError err(c05::Errc code, std::size_t offset, std::string_view field)
{
    return c05::DataError{code, offset, c05::OffsetUnit::byte, std::string(field)};
}

template<class T>
inline std::expected<T, c05::DataError> parse_int(std::string_view text, std::string_view field)
{
    if (text.empty()) {
        return std::unexpected(err(c05::Errc::invalid_number, 0, field));
    }
    T value{};
    const auto* first = text.data();
    const auto* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec == std::errc::invalid_argument) {
        return std::unexpected(err(c05::Errc::invalid_number, 0, field));
    }
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(err(c05::Errc::out_of_range, 0, field));
    }
    if (ptr != last) {
        return std::unexpected(err(c05::Errc::trailing_data, static_cast<std::size_t>(ptr - first), field));
    }
    return value;
}

inline std::expected<std::uint32_t, c05::DataError> parse_u32(std::string_view text, std::string_view field = "number")
{
    return parse_int<std::uint32_t>(text, field);
}
inline std::expected<std::int64_t, c05::DataError> parse_i64(std::string_view text, std::string_view field = "number")
{
    return parse_int<std::int64_t>(text, field);
}
inline std::expected<double, c05::DataError> parse_finite_double(std::string_view text, std::string_view field = "number")
{
    if (text.empty()) {
        return std::unexpected(err(c05::Errc::invalid_number, 0, field));
    }
    double value{};
    const auto* first = text.data();
    const auto* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec == std::errc::invalid_argument) {
        return std::unexpected(err(c05::Errc::invalid_number, 0, field));
    }
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(err(c05::Errc::out_of_range, 0, field));
    }
    if (ptr != last) {
        return std::unexpected(err(c05::Errc::trailing_data, static_cast<std::size_t>(ptr - first), field));
    }
    if (!std::isfinite(value)) {
        return std::unexpected(err(c05::Errc::invalid_value, 0, field));
    }
    return value;
}
}
