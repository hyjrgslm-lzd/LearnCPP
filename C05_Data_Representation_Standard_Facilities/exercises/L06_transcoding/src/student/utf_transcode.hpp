#pragma once
#include <expected>
#include <string>
#include <string_view>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<void, c05::DataError> validate_utf8(std::string_view)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "utf8"});
}
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(std::string_view s)
{
    (void)s;
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "utf8"});
}
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char8_t* s)
{
    return utf8_to_utf16(std::string_view(reinterpret_cast<const char*>(s)));
}
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char* s, std::size_t n)
{
    return utf8_to_utf16(std::string_view(s, n));
}
inline std::expected<std::string, c05::DataError> utf16_to_utf8(std::u16string_view)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::utf16_code_unit, "utf16"});
}
}
