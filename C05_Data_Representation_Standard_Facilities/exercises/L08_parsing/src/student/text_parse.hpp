#pragma once
#include <cstdint>
#include <expected>
#include <string_view>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<std::uint32_t, c05::DataError> parse_u32(std::string_view, std::string_view field = "number")
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, std::string(field)});
}
inline std::expected<std::int64_t, c05::DataError> parse_i64(std::string_view, std::string_view field = "number")
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, std::string(field)});
}
inline std::expected<double, c05::DataError> parse_finite_double(std::string_view, std::string_view field = "number")
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, std::string(field)});
}
}
