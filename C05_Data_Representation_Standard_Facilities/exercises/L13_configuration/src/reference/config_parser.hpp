#pragma once
#include <expected>
#include <string_view>

#include <c05/config.hpp>
#include <c05/model.hpp>

namespace c05_ex {
inline std::expected<c05::Config, c05::DataError> parse_config(std::string_view text)
{
    return c05::parse_config(text);
}
}
