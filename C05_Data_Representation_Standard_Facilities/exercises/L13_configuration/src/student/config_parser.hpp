#pragma once
#include <expected>
#include <string_view>

#include <c05/model.hpp>
#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<c05::Config, c05::DataError> parse_config(std::string_view)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "config"});
}
}
