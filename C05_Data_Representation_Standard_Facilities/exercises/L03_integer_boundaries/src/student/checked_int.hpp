#pragma once
#include <cstdint>
#include <expected>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<std::uint32_t, c05::DataError> checked_cast_u32(std::uint64_t)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "u32"});
}
}
