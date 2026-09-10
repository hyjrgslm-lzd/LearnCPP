#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <expected>
#include <string>
#include <string_view>

#include <c05/model.hpp>
#include <c05/utf.hpp>

namespace c05_ex {
namespace detail {
inline std::string_view trim(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) {
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

inline bool reserved(std::string_view s)
{
    auto stem = s.substr(0, s.find('.'));
    while (!stem.empty() && (stem.back() == ' ' || stem.back() == '.')) {
        stem.remove_suffix(1);
    }
    std::string u;
    for (char c : stem) {
        u.push_back(ascii_upper(c));
    }
    constexpr std::array names{"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"};
    return std::find(names.begin(), names.end(), u) != names.end()
        || (u.size() == 4 && ((u.starts_with("COM") || u.starts_with("LPT")) && u[3] >= '1' && u[3] <= '9'))
        || (u.size() == 5 && (u.starts_with("COM") || u.starts_with("LPT"))
            && static_cast<unsigned char>(u[3]) == 0xc2
            && (static_cast<unsigned char>(u[4]) == 0xb9 || static_cast<unsigned char>(u[4]) == 0xb2
                || static_cast<unsigned char>(u[4]) == 0xb3));
}

inline std::expected<void, c05::DataError> filename(std::string_view v, std::size_t off, std::size_t line)
{
    if (v.empty() || v == "." || v == ".." || v.contains('/') || v.contains('\\')
        || (v.size() > 1 && std::isalpha(static_cast<unsigned char>(v[0])) != 0 && v[1] == ':')
        || v.back() == ' ' || v.back() == '.' || reserved(v)) {
        return std::unexpected(c05::DataError{c05::Errc::invalid_config, off, c05::OffsetUnit::byte, "package_file", line});
    }
    for (std::size_t i = 0; i != v.size(); ++i) {
        const auto c = static_cast<unsigned char>(v[i]);
        if (c < 0x20 || v[i] == '<' || v[i] == '>' || v[i] == ':' || v[i] == '"' || v[i] == '|' || v[i] == '?' || v[i] == '*') {
            return std::unexpected(c05::DataError{c05::Errc::invalid_config, off + i, c05::OffsetUnit::byte, "package_file", line});
        }
    }
    return {};
}
}

inline std::expected<c05::Config, c05::DataError> parse_config(std::string_view text)
{
    if (text.size() > 64 * 1024) {
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, 0, c05::OffsetUnit::byte, "config"});
    }
    if (auto ok = c05::validate_utf8(text); !ok) {
        auto err = ok.error();
        err.field = "config";
        err.line = detail::line_from_offset(text, err.offset);
        return std::unexpected(err);
    }
    if (auto nul = text.find('\0'); nul != std::string_view::npos) {
        return std::unexpected(c05::DataError{c05::Errc::invalid_config, nul, c05::OffsetUnit::byte, "config", detail::line_from_offset(text, nul)});
    }
    c05::Config cfg;
    std::size_t pos = text.starts_with("\xef\xbb\xbf") ? 3 : 0;
    std::size_t line = 1;
    while (pos <= text.size()) {
        const auto begin = pos;
        const auto lf = text.find('\n', pos);
        auto end = lf == std::string_view::npos ? text.size() : lf;
        if (lf != std::string_view::npos && end > begin && text[end - 1] == '\r') {
            --end;
        }
        if (const auto cr = text.substr(begin, end - begin).find('\r'); cr != std::string_view::npos) {
            return std::unexpected(c05::DataError{c05::Errc::invalid_config, begin + cr, c05::OffsetUnit::byte, "config", line});
        }
        if (end - begin > 4096) {
            return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, begin, c05::OffsetUnit::byte, "config", line});
        }
        const auto raw = text.substr(begin, end - begin);
        const auto trimmed = detail::trim(raw);
        const auto trimmed_off = begin + (trimmed.data() - raw.data());
        if (!trimmed.empty() && trimmed.front() != '#') {
            const auto eq = trimmed.find('=');
            if (eq == std::string_view::npos || eq == 0) {
                return std::unexpected(c05::DataError{c05::Errc::invalid_config, trimmed_off, c05::OffsetUnit::byte, "config", line});
            }
            const auto key = detail::trim(trimmed.substr(0, eq));
            const auto value = detail::trim(trimmed.substr(eq + 1));
            const auto key_off = trimmed_off + (key.data() - trimmed.data());
            const auto value_off = trimmed_off + (value.data() - trimmed.data());
            if (value.empty()) {
                return std::unexpected(c05::DataError{c05::Errc::invalid_config, value_off, c05::OffsetUnit::byte, std::string(key), line});
            }
            if (key == "package_file") {
                if (auto ok = detail::filename(value, value_off, line); !ok) {
                    return std::unexpected(ok.error());
                }
                cfg.package_file = std::string(value);
            } else if (key == "display_zone") {
                cfg.display_zone = std::string(value);
            } else {
                return std::unexpected(c05::DataError{c05::Errc::invalid_config, key_off, c05::OffsetUnit::byte, std::string(key), line});
            }
        }
        if (lf == std::string_view::npos) {
            break;
        }
        pos = lf + 1;
        ++line;
    }
    return cfg;
}
}
