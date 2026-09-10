#pragma once
#include <cstddef>
#include <expected>
#include <span>
#include <string>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<std::string, c05::DataError> read_utf8_field(std::span<const std::byte>, std::size_t&)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "utf8_field"});
}
}
