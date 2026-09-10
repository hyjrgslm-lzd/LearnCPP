#pragma once
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<void, c05::DataError> valid_utf8(std::string_view s)
{
    auto byte = [](char c) { return static_cast<unsigned char>(c); };
    for (std::size_t i = 0; i < s.size();) {
        const auto start = i;
        const auto b0 = byte(s[i++]);
        if (b0 <= 0x7f) {
            continue;
        }
        int need = 0;
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
            return std::unexpected(c05::DataError{c05::Errc::invalid_encoding, start, c05::OffsetUnit::byte, "utf8_field"});
        }
        if (s.size() - i < static_cast<std::size_t>(need)) {
            return std::unexpected(c05::DataError{c05::Errc::incomplete_input, start, c05::OffsetUnit::byte, "utf8_field"});
        }
        for (int n = 0; n != need; ++n) {
            const auto bx = byte(s[i++]);
            if ((bx & 0xc0) != 0x80) {
                return std::unexpected(c05::DataError{c05::Errc::invalid_encoding, start, c05::OffsetUnit::byte, "utf8_field"});
            }
            cp = (cp << 6) | (bx & 0x3f);
        }
        if ((need == 2 && cp < 0x800) || (need == 3 && cp < 0x10000) || (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) {
            return std::unexpected(c05::DataError{c05::Errc::invalid_encoding, start, c05::OffsetUnit::byte, "utf8_field"});
        }
    }
    return {};
}

inline std::expected<std::string, c05::DataError> read_utf8_field(std::span<const std::byte> input, std::size_t& cursor)
{
    auto u8 = [](std::byte b) { return static_cast<std::uint32_t>(std::to_integer<unsigned char>(b)); };
    auto fail = [](c05::Errc code, std::size_t offset) {
        return std::expected<std::string, c05::DataError>(std::unexpected(c05::DataError{code, offset, c05::OffsetUnit::byte, "utf8_field"}));
    };
    auto probe = cursor;
    if (probe > input.size() || input.size() - probe < 4) {
        return fail(c05::Errc::incomplete_input, probe);
    }
    auto len = (u8(input[probe]) << 24) | (u8(input[probe + 1]) << 16) | (u8(input[probe + 2]) << 8) | u8(input[probe + 3]);
    probe += 4;
    if (len > c05::max_text_bytes) {
        return fail(c05::Errc::limit_exceeded, cursor);
    }
    if (input.size() - probe < len) {
        return fail(c05::Errc::incomplete_input, probe);
    }
    const auto text_start = probe;
    std::string text;
    text.reserve(len);
    for (std::uint32_t i = 0; i != len; ++i) {
        text.push_back(static_cast<char>(std::to_integer<unsigned char>(input[probe + i])));
    }
    auto ok = valid_utf8(text);
    if (!ok) {
        auto err = ok.error();
        err.offset += text_start;
        return std::unexpected(err);
    }
    cursor = probe + len;
    return text;
}
}
