#pragma once
#include <cstdint>
#include <expected>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<std::uint32_t, c05::DataError> checked_cast_u32(std::uint64_t value)
{
    return static_cast<std::uint32_t>(value);
}
}
