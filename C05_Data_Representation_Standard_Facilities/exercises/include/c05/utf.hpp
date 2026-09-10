#pragma once
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include <c05/types.hpp>

namespace c05 {
namespace detail {
inline std::expected<char32_t, DataError> decode_one_utf8(std::string_view s, std::size_t& i)
{
    const auto start = i;
    const auto byte = [](char c) { return static_cast<unsigned char>(c); };
    const auto fail = [&](Errc code) {
        return std::expected<char32_t, DataError>(
            std::unexpected(DataError{code, start, OffsetUnit::byte, "utf8"}));
    };
    if (i >= s.size()) {
        return fail(Errc::incomplete_input);
    }
    const auto b0 = byte(s[i++]);
    if (b0 <= 0x7f) {
        return static_cast<char32_t>(b0);
    }
    auto need = 0;
    char32_t cp = 0;
    if (b0 >= 0xc2 && b0 <= 0xdf) {
        need = 1;
        cp = b0 & 0x1f;
    } else if (b0 >= 0xe0 && b0 <= 0xef) {
        need = 2;
        cp = b0 & 0x0f;
    } else if (b0 >= 0xf0 && b0 <= 0xf4) {
        need = 3;
        cp = b0 & 0x07;
    } else {
        return fail(Errc::invalid_encoding);
    }
    if (s.size() - i < static_cast<std::size_t>(need)) {
        return fail(Errc::incomplete_input);
    }
    for (int n = 0; n != need; ++n) {
        const auto bx = byte(s[i++]);
        if ((bx & 0xc0) != 0x80) {
            return fail(Errc::invalid_encoding);
        }
        cp = (cp << 6) | (bx & 0x3f);
    }
    if ((need == 2 && cp < 0x800) || (need == 3 && cp < 0x10000)
        || (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) {
        return fail(Errc::invalid_encoding);
    }
    return cp;
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
}

inline std::expected<void, DataError> validate_utf8(std::string_view s)
{
    if (s.size() > max_package_bytes) {
        return std::unexpected(DataError{Errc::limit_exceeded, 0, OffsetUnit::byte, "utf8"});
    }
    for (std::size_t i = 0; i < s.size();) {
        auto cp = detail::decode_one_utf8(s, i);
        if (!cp) {
            return std::unexpected(cp.error());
        }
    }
    return {};
}

inline std::expected<std::u16string, DataError> utf8_to_utf16(std::string_view s)
{
    if (s.size() > max_package_bytes) {
        return std::unexpected(DataError{Errc::limit_exceeded, 0, OffsetUnit::byte, "utf8"});
    }
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto start = i;
        auto cp = detail::decode_one_utf8(s, i);
        if (!cp) {
            return std::unexpected(cp.error());
        }
        const auto need_units = *cp <= 0xffff ? std::size_t{1} : std::size_t{2};
        if (out.size() > max_package_bytes / sizeof(char16_t) - need_units) {
            return std::unexpected(DataError{Errc::limit_exceeded, start, OffsetUnit::byte, "utf8"});
        }
        if (*cp <= 0xffff) {
            out.push_back(static_cast<char16_t>(*cp));
        } else {
            const auto v = *cp - 0x10000;
            out.push_back(static_cast<char16_t>(0xd800 + (v >> 10)));
            out.push_back(static_cast<char16_t>(0xdc00 + (v & 0x3ff)));
        }
    }
    return out;
}

inline std::expected<std::string, DataError> utf16_to_utf8(std::u16string_view s)
{
    if (s.size() > max_package_bytes / sizeof(char16_t)) {
        return std::unexpected(DataError{Errc::limit_exceeded, 0, OffsetUnit::utf16_code_unit, "utf16"});
    }
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto start = i;
        char32_t cp = s[i];
        if (cp >= 0xd800 && cp <= 0xdbff) {
            if (i + 1 == s.size()) {
                return std::unexpected(DataError{Errc::incomplete_input, i, OffsetUnit::utf16_code_unit, "utf16"});
            }
            const auto lo = static_cast<char32_t>(s[++i]);
            if (lo < 0xdc00 || lo > 0xdfff) {
                return std::unexpected(DataError{Errc::invalid_encoding, i - 1, OffsetUnit::utf16_code_unit, "utf16"});
            }
            cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
        } else if (cp >= 0xdc00 && cp <= 0xdfff) {
            return std::unexpected(DataError{Errc::invalid_encoding, i, OffsetUnit::utf16_code_unit, "utf16"});
        }
        const auto need_bytes = cp <= 0x7f ? std::size_t{1} : cp <= 0x7ff ? std::size_t{2} : cp <= 0xffff ? std::size_t{3} : std::size_t{4};
        if (out.size() > max_package_bytes - need_bytes) {
            return std::unexpected(DataError{Errc::limit_exceeded, start, OffsetUnit::utf16_code_unit, "utf16"});
        }
        detail::append_utf8(out, cp);
    }
    return out;
}
}
