#pragma once
#include <c05/utf.hpp>

namespace c05_ex {
using c05::utf16_to_utf8;
using c05::utf8_to_utf16;
using c05::validate_utf8;
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char8_t* s)
{
    return c05::utf8_to_utf16(std::string_view(reinterpret_cast<const char*>(s)));
}
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char* s, std::size_t n)
{
    return c05::utf8_to_utf16(std::string_view(s, n));
}
}
