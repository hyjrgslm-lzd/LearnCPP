#pragma once
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <c05/bytes.hpp>
#include <c05/paths.hpp>
#include <c05/model.hpp>
#include <c05/types.hpp>
#include <c05/utf.hpp>

namespace c05_ex {
namespace detail {
inline c05::DataError err(c05::Errc code, std::size_t offset, std::string field)
{
    return c05::DataError{code, offset, c05::OffsetUnit::byte, std::move(field)};
}

inline std::expected<void, c05::DataError> text_ok(std::string_view s, std::string field, bool empty_ok)
{
    if (s.size() > c05::max_text_bytes) {
        return std::unexpected(err(c05::Errc::limit_exceeded, 0, std::move(field)));
    }
    if (!empty_ok && s.empty()) {
        return std::unexpected(err(c05::Errc::invalid_value, 0, std::move(field)));
    }
    if (s.find('\0') != std::string_view::npos) {
        return std::unexpected(err(c05::Errc::invalid_value, 0, std::move(field)));
    }
    auto utf = c05::validate_utf8(s);
    if (!utf) {
        auto e = utf.error();
        e.field = std::move(field);
        return std::unexpected(e);
    }
    return {};
}

inline std::string to_string(std::span<const std::byte> bytes)
{
    std::string out;
    out.reserve(bytes.size());
    for (auto b : bytes) {
        out.push_back(static_cast<char>(std::to_integer<unsigned char>(b)));
    }
    return out;
}

inline std::expected<void, c05::DataError> room(std::vector<std::byte>& out, std::size_t n)
{
    if (out.size() > c05::max_package_bytes || n > c05::max_package_bytes - out.size()) {
        return std::unexpected(err(c05::Errc::limit_exceeded, out.size(), "package"));
    }
    return {};
}

inline std::expected<void, c05::DataError> append_text(std::vector<std::byte>& out, std::uint16_t tag, std::string_view s)
{
    if (s.size() > c05::max_text_bytes) {
        return std::unexpected(err(c05::Errc::limit_exceeded, out.size(), "text"));
    }
    if (auto ok = room(out, 6 + s.size()); !ok) {
        return ok;
    }
    c05::append_be(out, tag);
    c05::append_be<std::uint32_t>(out, static_cast<std::uint32_t>(s.size()));
    for (unsigned char c : s) {
        out.push_back(static_cast<std::byte>(c));
    }
    return {};
}

template<class T>
inline std::expected<void, c05::DataError> append_num(std::vector<std::byte>& out, std::uint16_t tag, T value)
{
    if (auto ok = room(out, 6 + sizeof(T)); !ok) {
        return ok;
    }
    c05::append_be(out, tag);
    c05::append_be<std::uint32_t>(out, static_cast<std::uint32_t>(sizeof(T)));
    if constexpr (std::same_as<T, std::int64_t>) {
        c05::append_be(out, std::bit_cast<std::uint64_t>(value));
    } else {
        c05::append_be(out, value);
    }
    return {};
}
}

inline std::expected<void, c05::DataError> validate_manifest(const c05::Manifest& m)
{
    if (m.records.size() > c05::max_records) {
        return std::unexpected(detail::err(c05::Errc::limit_exceeded, 0, "records"));
    }
    std::unordered_set<std::uint32_t> ids;
    for (const auto& r : m.records) {
        if (r.id == 0) {
            return std::unexpected(detail::err(c05::Errc::invalid_value, 0, "id"));
        }
        if (!ids.insert(r.id).second) {
            return std::unexpected(detail::err(c05::Errc::duplicate_field, 0, "id"));
        }
        if (r.modified_at_ms < c05::min_timestamp_ms || r.modified_at_ms > c05::max_timestamp_ms) {
            return std::unexpected(detail::err(c05::Errc::out_of_range, 0, "mtime"));
        }
        if (auto ok = detail::text_ok(r.name, "name", false); !ok) {
            return ok;
        }
        if (auto ok = detail::text_ok(r.path, "path", false); !ok) {
            return ok;
        }
        if (auto ok = c05::validate_resource_path(r.path); !ok) {
            return ok;
        }
        if (r.note) {
            if (auto ok = detail::text_ok(*r.note, "note", true); !ok) {
                return ok;
            }
        }
    }
    return {};
}

inline std::expected<std::vector<std::byte>, c05::DataError> encode_manifest(
    const c05::Manifest& m, c05::WireVersion version = c05::WireVersion::v2)
{
    if (version != c05::WireVersion::v1 && version != c05::WireVersion::v2) {
        return std::unexpected(detail::err(c05::Errc::unsupported_version, 0, "version"));
    }
    if (auto ok = validate_manifest(m); !ok) {
        return std::unexpected(ok.error());
    }
    if (version == c05::WireVersion::v1) {
        for (const auto& r : m.records) {
            if (r.note) {
                return std::unexpected(detail::err(c05::Errc::unsupported_version, 0, "note"));
            }
        }
    }
    std::vector<std::byte> out;
    out.reserve(12);
    for (char c : std::string_view("C05M", 4)) {
        out.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    }
    c05::append_be<std::uint16_t>(out, 1);
    c05::append_be<std::uint16_t>(out, static_cast<std::uint16_t>(version));
    c05::append_be<std::uint32_t>(out, static_cast<std::uint32_t>(m.records.size()));
    for (const auto& r : m.records) {
        std::vector<std::byte> body;
        if (auto ok = detail::append_num(body, 1, r.id); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = detail::append_text(body, 2, r.name); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = detail::append_text(body, 3, r.path); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = detail::append_num(body, 4, r.byte_size); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = detail::append_num(body, 5, r.modified_at_ms); !ok) {
            return std::unexpected(ok.error());
        }
        if (version == c05::WireVersion::v2 && r.note) {
            if (auto ok = detail::append_text(body, 6, *r.note); !ok) {
                return std::unexpected(ok.error());
            }
        }
        if (auto ok = detail::room(out, 4 + body.size()); !ok) {
            return std::unexpected(ok.error());
        }
        c05::append_be<std::uint32_t>(out, static_cast<std::uint32_t>(body.size()));
        out.insert(out.end(), body.begin(), body.end());
    }
    return out;
}

inline std::expected<c05::Manifest, c05::DataError> decode_manifest(std::span<const std::byte> input)
{
    if (input.size() > c05::max_package_bytes || input.size() < 12) {
        return std::unexpected(detail::err(c05::Errc::incomplete_input, 0, "header"));
    }
    if (input[0] != std::byte{0x43} || input[1] != std::byte{0x30} || input[2] != std::byte{0x35} || input[3] != std::byte{0x4d}) {
        return std::unexpected(detail::err(c05::Errc::invalid_value, 0, "magic"));
    }
    std::size_t cursor = 4;
    auto major = c05::read_be<std::uint16_t>(input, cursor);
    auto minor = c05::read_be<std::uint16_t>(input, cursor);
    auto count = c05::read_be<std::uint32_t>(input, cursor);
    if (!major || !minor || !count || *major != 1 || *minor == 0) {
        return std::unexpected(detail::err(c05::Errc::unsupported_version, 4, "version"));
    }
    if (*count > c05::max_records) {
        return std::unexpected(detail::err(c05::Errc::limit_exceeded, 8, "records"));
    }
    c05::Manifest m;
    std::unordered_set<std::uint32_t> ids;
    for (std::uint32_t n = 0; n != *count; ++n) {
        const auto record_start = cursor;
        auto body_len = c05::read_be<std::uint32_t>(input, cursor);
        if (!body_len || input.size() - cursor < *body_len) {
            return std::unexpected(detail::err(c05::Errc::incomplete_input, record_start, "record"));
        }
        const auto end = cursor + *body_len;
        c05::ResourceRecord r;
        bool seen_id = false, seen_name = false, seen_path = false, seen_size = false, seen_mtime = false;
        std::unordered_set<std::uint16_t> unknown;
        while (cursor < end) {
            const auto field_start = cursor;
            auto tag = c05::read_be<std::uint16_t>(input, cursor);
            auto len = c05::read_be<std::uint32_t>(input, cursor);
            if (!tag || !len || end - cursor < *len) {
                return std::unexpected(detail::err(c05::Errc::incomplete_input, field_start, "field"));
            }
            auto payload = input.subspan(cursor, *len);
            cursor += *len;
            auto dup = [&](bool& seen, std::string field) -> std::expected<void, c05::DataError> {
                if (seen) {
                    return std::unexpected(detail::err(c05::Errc::duplicate_field, field_start, std::move(field)));
                }
                seen = true;
                return {};
            };
            switch (*tag) {
            case 1: {
                if (auto ok = dup(seen_id, "id"); !ok) {
                    return std::unexpected(ok.error());
                }
                if (payload.size() != 4) {
                    return std::unexpected(detail::err(c05::Errc::invalid_value, field_start, "id"));
                }
                std::size_t p = 0;
                r.id = *c05::read_be<std::uint32_t>(payload, p);
                break;
            }
            case 2:
                if (auto ok = dup(seen_name, "name"); !ok) {
                    return std::unexpected(ok.error());
                }
                r.name = detail::to_string(payload);
                if (auto ok = detail::text_ok(r.name, "name", false); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            case 3:
                if (auto ok = dup(seen_path, "path"); !ok) {
                    return std::unexpected(ok.error());
                }
                r.path = detail::to_string(payload);
                if (auto ok = detail::text_ok(r.path, "path", false); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                if (auto ok = c05::validate_resource_path(r.path); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            case 4: {
                if (auto ok = dup(seen_size, "size"); !ok) {
                    return std::unexpected(ok.error());
                }
                if (payload.size() != 8) {
                    return std::unexpected(detail::err(c05::Errc::invalid_value, field_start, "size"));
                }
                std::size_t p = 0;
                r.byte_size = *c05::read_be<std::uint64_t>(payload, p);
                break;
            }
            case 5: {
                if (auto ok = dup(seen_mtime, "mtime"); !ok) {
                    return std::unexpected(ok.error());
                }
                if (payload.size() != 8) {
                    return std::unexpected(detail::err(c05::Errc::invalid_value, field_start, "mtime"));
                }
                std::size_t p = 0;
                auto raw = c05::read_be<std::uint64_t>(payload, p);
                r.modified_at_ms = std::bit_cast<std::int64_t>(*raw);
                break;
            }
            case 6:
                if (*minor < 2 || r.note) {
                    return std::unexpected(detail::err(r.note ? c05::Errc::duplicate_field : c05::Errc::unsupported_version, field_start, "note"));
                }
                r.note = detail::to_string(payload);
                if (auto ok = detail::text_ok(*r.note, "note", true); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            default:
                if (!unknown.insert(*tag).second) {
                    return std::unexpected(detail::err(c05::Errc::duplicate_field, field_start, "unknown"));
                }
                break;
            }
        }
        if (!(seen_id && seen_name && seen_path && seen_size && seen_mtime)) {
            return std::unexpected(detail::err(c05::Errc::missing_field, record_start, "record"));
        }
        if (r.id == 0 || !ids.insert(r.id).second) {
            return std::unexpected(detail::err(c05::Errc::duplicate_field, record_start, "id"));
        }
        if (r.modified_at_ms < c05::min_timestamp_ms || r.modified_at_ms > c05::max_timestamp_ms) {
            return std::unexpected(detail::err(c05::Errc::out_of_range, record_start, "mtime"));
        }
        m.records.push_back(std::move(r));
    }
    if (cursor != input.size()) {
        return std::unexpected(detail::err(c05::Errc::trailing_data, cursor, "package"));
    }
    return m;
}
}
