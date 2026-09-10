#pragma once
#include <cctype>
#include <expected>
#include <string_view>

#include <c05/types.hpp>
#include <c05/utf.hpp>

namespace c05 {
inline std::expected<void, DataError> validate_resource_path(std::string_view path)
{
    if (path.empty()) {
        return std::unexpected(DataError{Errc::invalid_value, 0, OffsetUnit::byte, "path"});
    }
    if (path.size() > max_text_bytes) {
        return std::unexpected(DataError{Errc::limit_exceeded, 0, OffsetUnit::byte, "path"});
    }
    if (auto ok = validate_utf8(path); !ok) {
        auto err = ok.error();
        err.field = "path";
        return std::unexpected(err);
    }
    if (path.front() == '/') {
        return std::unexpected(DataError{Errc::invalid_value, 0, OffsetUnit::byte, "path"});
    }
    for (std::size_t i = 0; i != path.size(); ++i) {
        const auto c = static_cast<unsigned char>(path[i]);
        if (c == 0 || c == '\\') {
            return std::unexpected(DataError{Errc::invalid_value, i, OffsetUnit::byte, "path"});
        }
        if (i == 1 && path[i] == ':' && std::isalpha(static_cast<unsigned char>(path[0])) != 0) {
            return std::unexpected(DataError{Errc::invalid_value, 0, OffsetUnit::byte, "path"});
        }
    }
    for (std::size_t start = 0; start <= path.size();) {
        const auto slash = path.find('/', start);
        const auto end = slash == std::string_view::npos ? path.size() : slash;
        const auto part = path.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") {
            return std::unexpected(DataError{Errc::invalid_value, start, OffsetUnit::byte, "path"});
        }
        if (slash == std::string_view::npos) {
            break;
        }
        start = slash + 1;
    }
    return {};
}
}
