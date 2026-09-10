#pragma once
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

#include <c05/types.hpp>
#include <c05/utf.hpp>

namespace c05_ex {
inline std::expected<std::string, c05::DataError> read_utf8_field(std::span<const std::byte> input, std::size_t& cursor)
{
    auto probe = cursor;
    if (probe > input.size() || input.size() - probe < 4) {
        return std::unexpected(c05::DataError{c05::Errc::incomplete_input, probe, c05::OffsetUnit::byte, "utf8_field"});
    }
    auto u8 = [](std::byte b) { return static_cast<std::uint32_t>(std::to_integer<unsigned char>(b)); };
    auto len = (u8(input[probe]) << 24) | (u8(input[probe + 1]) << 16) | (u8(input[probe + 2]) << 8) | u8(input[probe + 3]);
    probe += 4;
    if (len > c05::max_text_bytes) {
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, cursor, c05::OffsetUnit::byte, "utf8_field"});
    }
    // Deliberate defect: commit the header before the rest can succeed.
    cursor = probe;
    const auto text_start = probe;
    if (probe > input.size() || input.size() - probe < len) {
        return std::unexpected(c05::DataError{c05::Errc::incomplete_input, probe, c05::OffsetUnit::byte, "utf8_field"});
    }
    std::string text;
    for (std::uint32_t i = 0; i != len; ++i) {
        text.push_back(static_cast<char>(std::to_integer<unsigned char>(input[probe++])));
    }
    if (auto valid = c05::validate_utf8(text); !valid) {
        auto error = valid.error();
        error.offset += text_start;
        error.field = "utf8_field";
        return std::unexpected(error);
    }
    cursor = probe;
    return text;
}
}
