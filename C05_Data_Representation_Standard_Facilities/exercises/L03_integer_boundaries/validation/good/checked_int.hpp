#pragma once
#include <cstdint>
#include <expected>
#include <limits>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<std::uint32_t, c05::DataError> checked_cast_u32(std::uint64_t value)
{
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(c05::DataError{c05::Errc::out_of_range, 0, c05::OffsetUnit::byte, "u32"});
    }
    return static_cast<std::uint32_t>(value);
}
}
