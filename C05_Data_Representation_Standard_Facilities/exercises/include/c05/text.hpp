#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <string_view>

#include <c05/types.hpp>

namespace c05 {
namespace text_detail {
inline DataError number_error(Errc code, std::size_t offset, std::string_view field)
{
    return DataError{code, offset, OffsetUnit::byte, std::string(field)};
}

template<class T>
inline std::expected<T, DataError> parse_integer(std::string_view text, std::string_view field)
{
    if (text.empty()) {
        return std::unexpected(number_error(Errc::invalid_number, 0, field));
    }
    T value{};
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(first, last, value, 10);
    if (ec == std::errc::invalid_argument) {
        return std::unexpected(number_error(Errc::invalid_number, 0, field));
    }
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(number_error(Errc::out_of_range, 0, field));
    }
    if (ptr != last) {
        return std::unexpected(number_error(Errc::trailing_data, static_cast<std::size_t>(ptr - first), field));
    }
    return value;
}
}

inline std::expected<std::uint32_t, DataError> parse_u32(std::string_view text, std::string_view field = "number")
{
    return text_detail::parse_integer<std::uint32_t>(text, field);
}

inline std::expected<std::int64_t, DataError> parse_i64(std::string_view text, std::string_view field = "number")
{
    return text_detail::parse_integer<std::int64_t>(text, field);
}

inline std::expected<double, DataError> parse_finite_double(std::string_view text, std::string_view field = "number")
{
    if (text.empty()) {
        return std::unexpected(text_detail::number_error(Errc::invalid_number, 0, field));
    }
    double value{};
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec == std::errc::invalid_argument) {
        return std::unexpected(text_detail::number_error(Errc::invalid_number, 0, field));
    }
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(text_detail::number_error(Errc::out_of_range, 0, field));
    }
    if (ptr != last) {
        return std::unexpected(text_detail::number_error(Errc::trailing_data, static_cast<std::size_t>(ptr - first), field));
    }
    if (!std::isfinite(value)) {
        return std::unexpected(text_detail::number_error(Errc::invalid_value, 0, field));
    }
    return value;
}
}
