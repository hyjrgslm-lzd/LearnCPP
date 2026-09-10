#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <expected>
#include <string>
#include <string_view>

#include <c05/model.hpp>
#include <c05/types.hpp>
#include <c05/utf.hpp>

namespace c05 {
namespace detail {
inline std::string_view trim_ascii(std::string_view s)
{
    const auto is_edge = [](unsigned char c) { return c == ' ' || c == '\t'; };
    while (!s.empty() && is_edge(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && is_edge(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

inline char ascii_upper(char c)
{
    return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
}

inline std::size_t line_from_offset(std::string_view text, std::size_t offset)
{
    std::size_t line = 1;
    for (std::size_t i = 0; i < offset && i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++line;
        }
    }
    return line;
}

inline bool is_windows_device_name(std::string_view name)
{
    auto stem = name.substr(0, name.find('.'));
    while (!stem.empty() && (stem.back() == ' ' || stem.back() == '.')) {
        stem.remove_suffix(1);
    }
    std::string upper;
    upper.reserve(stem.size());
    for (char c : stem) {
        upper.push_back(ascii_upper(c));
    }
    constexpr std::array reserved{"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"};
    if (std::find(reserved.begin(), reserved.end(), upper) != reserved.end()) {
        return true;
    }
    if (upper.size() == 4
        && ((upper.starts_with("COM") && upper[3] >= '1' && upper[3] <= '9')
            || (upper.starts_with("LPT") && upper[3] >= '1' && upper[3] <= '9'))) {
        return true;
    }
    const auto superscript = upper.size() == 5
        && static_cast<unsigned char>(upper[3]) == 0xc2
        && (static_cast<unsigned char>(upper[4]) == 0xb9 || static_cast<unsigned char>(upper[4]) == 0xb2
            || static_cast<unsigned char>(upper[4]) == 0xb3);
    return superscript && (upper.starts_with("COM") || upper.starts_with("LPT"));
}

inline std::expected<void, DataError> validate_package_file(std::string_view name, std::size_t offset, std::size_t line)
{
    if (name.empty()) {
        return std::unexpected(DataError{Errc::invalid_config, offset, OffsetUnit::byte, "package_file", line});
    }
    if (name == "." || name == ".." || name.contains('/') || name.contains('\\')
        || (name.size() >= 2 && std::isalpha(static_cast<unsigned char>(name[0])) != 0 && name[1] == ':')) {
        return std::unexpected(DataError{Errc::invalid_config, offset, OffsetUnit::byte, "package_file", line});
    }
    if (name.back() == ' ' || name.back() == '.') {
        return std::unexpected(DataError{Errc::invalid_config, offset + name.size() - 1, OffsetUnit::byte, "package_file", line});
    }
    for (std::size_t i = 0; i != name.size(); ++i) {
        const auto c = static_cast<unsigned char>(name[i]);
        if (c < 0x20 || c == '<' || c == '>' || c == ':' || c == '"' || c == '|' || c == '?' || c == '*') {
            return std::unexpected(DataError{Errc::invalid_config, offset + i, OffsetUnit::byte, "package_file", line});
        }
    }
    if (is_windows_device_name(name)) {
        return std::unexpected(DataError{Errc::invalid_config, offset, OffsetUnit::byte, "package_file", line});
    }
    return {};
}
}

inline std::expected<Config, DataError> parse_config(std::string_view text)
{
    if (text.size() > 64 * 1024) {
        return std::unexpected(DataError{Errc::limit_exceeded, 0, OffsetUnit::byte, "config"});
    }
    if (auto ok = validate_utf8(text); !ok) {
        auto err = ok.error();
        err.field = "config";
        err.line = detail::line_from_offset(text, err.offset);
        return std::unexpected(err);
    }
    if (const auto nul = text.find('\0'); nul != std::string_view::npos) {
        return std::unexpected(DataError{Errc::invalid_config, nul, OffsetUnit::byte, "config", detail::line_from_offset(text, nul)});
    }
    Config cfg;
    bool seen_package = false;
    bool seen_zone = false;
    std::size_t offset = 0;
    std::size_t line = 1;
    if (text.starts_with("\xef\xbb\xbf")) {
        offset = 3;
    }
    while (offset <= text.size()) {
        const auto line_start = offset;
        const auto lf = text.find('\n', offset);
        auto line_end = lf == std::string_view::npos ? text.size() : lf;
        if (lf != std::string_view::npos && line_end > line_start && text[line_end - 1] == '\r') {
            --line_end;
        }
        if (const auto cr = text.substr(line_start, line_end - line_start).find('\r'); cr != std::string_view::npos) {
            return std::unexpected(DataError{Errc::invalid_config, line_start + cr, OffsetUnit::byte, "config", line});
        }
        if (line_end - line_start > 4096) {
            return std::unexpected(DataError{Errc::limit_exceeded, line_start, OffsetUnit::byte, "config", line});
        }
        auto raw = text.substr(line_start, line_end - line_start);
        auto trimmed = detail::trim_ascii(raw);
        const auto trimmed_offset = line_start + (trimmed.data() - raw.data());
        if (!trimmed.empty() && trimmed.front() != '#') {
            const auto eq = trimmed.find('=');
            if (eq == std::string_view::npos || eq == 0) {
                return std::unexpected(DataError{Errc::invalid_config, trimmed_offset, OffsetUnit::byte, "config", line});
            }
            auto key = detail::trim_ascii(trimmed.substr(0, eq));
            auto value = detail::trim_ascii(trimmed.substr(eq + 1));
            const auto key_offset = trimmed_offset + (key.data() - trimmed.data());
            const auto value_offset = trimmed_offset + (value.data() - trimmed.data());
            if (value.empty()) {
                return std::unexpected(DataError{Errc::invalid_config, value_offset, OffsetUnit::byte, std::string(key), line});
            }
            if (key == "package_file") {
                if (seen_package) {
                    return std::unexpected(DataError{Errc::duplicate_field, key_offset, OffsetUnit::byte, "package_file", line});
                }
                if (auto ok = detail::validate_package_file(value, value_offset, line); !ok) {
                    return std::unexpected(ok.error());
                }
                cfg.package_file = std::string(value);
                seen_package = true;
            } else if (key == "display_zone") {
                if (seen_zone) {
                    return std::unexpected(DataError{Errc::duplicate_field, key_offset, OffsetUnit::byte, "display_zone", line});
                }
                cfg.display_zone = std::string(value);
                seen_zone = true;
            } else {
                return std::unexpected(DataError{Errc::invalid_config, key_offset, OffsetUnit::byte, std::string(key), line});
            }
        }
        if (lf == std::string_view::npos) {
            break;
        }
        offset = lf + 1;
        ++line;
    }
    return cfg;
}
}
