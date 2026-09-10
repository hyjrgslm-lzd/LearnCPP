#pragma once
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<char32_t, c05::DataError> next_utf8(std::string_view s, std::size_t& i)
{
    const auto start = i;
    const auto u = [](char c) { return static_cast<unsigned char>(c); };
    const auto bad = [&](c05::Errc code) {
        return std::expected<char32_t, c05::DataError>(std::unexpected(c05::DataError{code, start, c05::OffsetUnit::byte, "utf8"}));
    };
    const auto b0 = u(s[i++]);
    if (b0 <= 0x7f) {
        return b0;
    }
    int n = 0;
    char32_t cp = 0;
    if (b0 >= 0xc2 && b0 <= 0xdf) {
        n = 1;
        cp = b0 & 0x1f;
    } else if (b0 >= 0xe0 && b0 <= 0xef) {
        n = 2;
        cp = b0 & 0x0f;
    } else if (b0 >= 0xf0 && b0 <= 0xf4) {
        n = 3;
        cp = b0 & 0x07;
    } else {
        return bad(c05::Errc::invalid_encoding);
    }
    if (s.size() - i < static_cast<std::size_t>(n)) {
        return bad(c05::Errc::incomplete_input);
    }
    for (int k = 0; k != n; ++k) {
        const auto bx = u(s[i++]);
        if ((bx & 0xc0) != 0x80) {
            return bad(c05::Errc::invalid_encoding);
        }
        cp = (cp << 6) | (bx & 0x3f);
    }
    if ((n == 2 && cp < 0x800) || (n == 3 && cp < 0x10000) || (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) {
        return bad(c05::Errc::invalid_encoding);
    }
    return cp;
}

inline std::expected<void, c05::DataError> validate_utf8(std::string_view s)
{
    if (s.size() > c05::max_package_bytes) {
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, 0, c05::OffsetUnit::byte, "utf8"});
    }
    for (std::size_t i = 0; i < s.size();) {
        auto cp = next_utf8(s, i);
        if (!cp) {
            return std::unexpected(cp.error());
        }
    }
    return {};
}

inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(std::string_view s)
{
    if (s.size() > c05::max_package_bytes) {
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, 0, c05::OffsetUnit::byte, "utf8"});
    }
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto start = i;
        auto cp = next_utf8(s, i);
        if (!cp) {
            return std::unexpected(cp.error());
        }
        const auto need_units = *cp <= 0xffff ? std::size_t{1} : std::size_t{2};
        if (out.size() > c05::max_package_bytes / sizeof(char16_t) - need_units) {
            return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, start, c05::OffsetUnit::byte, "utf8"});
        }
        if (*cp <= 0xffff) {
            out.push_back(static_cast<char16_t>(*cp));
        } else {
            auto v = *cp - 0x10000;
            out.push_back(static_cast<char16_t>(0xd800 + (v >> 10)));
            out.push_back(static_cast<char16_t>(0xdc00 + (v & 0x3ff)));
        }
    }
    return out;
}

inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char8_t* s)
{
    return utf8_to_utf16(std::string_view(reinterpret_cast<const char*>(s)));
}
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char* s, std::size_t n)
{
    return utf8_to_utf16(std::string_view(s, n));
}

inline void append_utf8(std::string& out, char32_t cp)
{
    if (cp <= 0x7f) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7ff) {
        out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else if (cp <= 0xffff) {
        out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else {
        out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    }
}

inline std::expected<std::string, c05::DataError> utf16_to_utf8(std::u16string_view s)
{
    if (s.size() > c05::max_package_bytes / sizeof(char16_t)) {
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, 0, c05::OffsetUnit::utf16_code_unit, "utf16"});
    }
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto start = i;
        char32_t cp = s[i];
        if (cp >= 0xd800 && cp <= 0xdbff) {
            if (i + 1 == s.size()) {
                return std::unexpected(c05::DataError{c05::Errc::incomplete_input, i, c05::OffsetUnit::utf16_code_unit, "utf16"});
            }
            auto lo = static_cast<char32_t>(s[++i]);
            if (lo < 0xdc00 || lo > 0xdfff) {
                return std::unexpected(c05::DataError{c05::Errc::invalid_encoding, i - 1, c05::OffsetUnit::utf16_code_unit, "utf16"});
            }
            cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
        } else if (cp >= 0xdc00 && cp <= 0xdfff) {
            return std::unexpected(c05::DataError{c05::Errc::invalid_encoding, i, c05::OffsetUnit::utf16_code_unit, "utf16"});
        }
        const auto need_bytes = cp <= 0x7f ? std::size_t{1} : cp <= 0x7ff ? std::size_t{2} : cp <= 0xffff ? std::size_t{3} : std::size_t{4};
        if (out.size() > c05::max_package_bytes - need_bytes) {
            return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, start, c05::OffsetUnit::utf16_code_unit, "utf16"});
        }
        append_utf8(out, cp);
    }
    return out;
}
}
