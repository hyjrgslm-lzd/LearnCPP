#pragma once
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
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

namespace c05 {
namespace manifest_detail {
inline constexpr std::uint16_t tag_id = 1;
inline constexpr std::uint16_t tag_name = 2;
inline constexpr std::uint16_t tag_path = 3;
inline constexpr std::uint16_t tag_size = 4;
inline constexpr std::uint16_t tag_mtime = 5;
inline constexpr std::uint16_t tag_note = 6;
inline constexpr std::size_t header_size = 12;
inline constexpr std::string_view magic = "C05M";

inline DataError err(Errc code, std::size_t offset, std::string field)
{
    return DataError{code, offset, OffsetUnit::byte, std::move(field)};
}

inline bool has_nul(std::string_view text)
{
    return text.find('\0') != std::string_view::npos;
}

inline std::expected<void, DataError> validate_text(
    std::string_view text, std::string_view field, bool allow_empty, bool allow_nul)
{
    if (text.size() > max_text_bytes) {
        return std::unexpected(err(Errc::limit_exceeded, 0, std::string(field)));
    }
    if (!allow_empty && text.empty()) {
        return std::unexpected(err(Errc::invalid_value, 0, std::string(field)));
    }
    if (!allow_nul && has_nul(text)) {
        return std::unexpected(err(Errc::invalid_value, 0, std::string(field)));
    }
    auto ok = validate_utf8(text);
    if (!ok) {
        auto e = ok.error();
        e.field = std::string(field);
        return std::unexpected(e);
    }
    return {};
}

inline std::expected<void, DataError> append_checked(std::vector<std::byte>& out, std::size_t extra)
{
    if (out.size() > max_package_bytes || extra > max_package_bytes - out.size()) {
        return std::unexpected(err(Errc::limit_exceeded, out.size(), "package"));
    }
    return {};
}

template<class T>
inline std::expected<void, DataError> append_number_field(std::vector<std::byte>& out, std::uint16_t tag, T value)
{
    if (auto ok = append_checked(out, 2 + 4 + sizeof(T)); !ok) {
        return ok;
    }
    append_be(out, tag);
    append_be<std::uint32_t>(out, static_cast<std::uint32_t>(sizeof(T)));
    if constexpr (std::same_as<T, std::int64_t>) {
        append_be(out, std::bit_cast<std::uint64_t>(value));
    } else {
        append_be(out, value);
    }
    return {};
}

inline std::expected<void, DataError> append_text_field(
    std::vector<std::byte>& out, std::uint16_t tag, std::string_view text)
{
    if (text.size() > max_text_bytes || text.size() > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(err(Errc::limit_exceeded, out.size(), "text"));
    }
    if (auto ok = append_checked(out, 2 + 4 + text.size()); !ok) {
        return ok;
    }
    append_be(out, tag);
    append_be<std::uint32_t>(out, static_cast<std::uint32_t>(text.size()));
    for (unsigned char c : text) {
        out.push_back(static_cast<std::byte>(c));
    }
    return {};
}

inline std::expected<std::uint64_t, DataError> read_u64_payload(
    std::span<const std::byte> payload, std::size_t offset, std::string field)
{
    if (payload.size() != sizeof(std::uint64_t)) {
        return std::unexpected(err(Errc::invalid_value, offset, std::move(field)));
    }
    std::size_t inner = 0;
    return read_be<std::uint64_t>(payload, inner);
}

inline std::expected<std::uint32_t, DataError> read_u32_payload(
    std::span<const std::byte> payload, std::size_t offset, std::string field)
{
    if (payload.size() != sizeof(std::uint32_t)) {
        return std::unexpected(err(Errc::invalid_value, offset, std::move(field)));
    }
    std::size_t inner = 0;
    return read_be<std::uint32_t>(payload, inner);
}

inline std::string bytes_to_string(std::span<const std::byte> payload)
{
    std::string text;
    text.reserve(payload.size());
    for (auto b : payload) {
        text.push_back(static_cast<char>(std::to_integer<unsigned char>(b)));
    }
    return text;
}
}

inline std::expected<void, DataError> validate_manifest(const Manifest& manifest)
{
    if (manifest.records.size() > max_records) {
        return std::unexpected(manifest_detail::err(Errc::limit_exceeded, 0, "records"));
    }
    std::unordered_set<std::uint32_t> ids;
    ids.reserve(manifest.records.size());
    for (std::size_t i = 0; i != manifest.records.size(); ++i) {
        const auto& r = manifest.records[i];
        const auto where = std::string("record[") + std::to_string(i) + "]";
        if (r.id == 0) {
            return std::unexpected(manifest_detail::err(Errc::invalid_value, 0, where + ".id"));
        }
        if (!ids.insert(r.id).second) {
            return std::unexpected(manifest_detail::err(Errc::duplicate_field, 0, where + ".id"));
        }
        if (r.modified_at_ms < min_timestamp_ms || r.modified_at_ms > max_timestamp_ms) {
            return std::unexpected(manifest_detail::err(Errc::out_of_range, 0, where + ".modified_at_ms"));
        }
        if (auto ok = manifest_detail::validate_text(r.name, where + ".name", false, false); !ok) {
            return ok;
        }
        if (auto ok = manifest_detail::validate_text(r.path, where + ".path", false, false); !ok) {
            return ok;
        }
        if (auto ok = validate_resource_path(r.path); !ok) {
            return ok;
        }
        if (r.note) {
            if (auto ok = manifest_detail::validate_text(*r.note, where + ".note", true, false); !ok) {
                return ok;
            }
        }
    }
    return {};
}

inline std::expected<std::vector<std::byte>, DataError> encode_manifest(
    const Manifest& manifest, WireVersion version = WireVersion::v2)
{
    if (version != WireVersion::v1 && version != WireVersion::v2) {
        return std::unexpected(manifest_detail::err(Errc::unsupported_version, 0, "version"));
    }
    if (auto ok = validate_manifest(manifest); !ok) {
        return std::unexpected(ok.error());
    }
    if (version == WireVersion::v1) {
        for (const auto& r : manifest.records) {
            if (r.note.has_value()) {
                return std::unexpected(manifest_detail::err(Errc::unsupported_version, 0, "note"));
            }
        }
    }

    std::vector<std::byte> out;
    out.reserve(manifest_detail::header_size);
    for (char c : manifest_detail::magic) {
        out.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    }
    append_be<std::uint16_t>(out, 1);
    append_be<std::uint16_t>(out, static_cast<std::uint16_t>(version));
    append_be<std::uint32_t>(out, static_cast<std::uint32_t>(manifest.records.size()));

    for (const auto& r : manifest.records) {
        std::vector<std::byte> body;
        if (auto ok = manifest_detail::append_number_field(body, manifest_detail::tag_id, r.id); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = manifest_detail::append_text_field(body, manifest_detail::tag_name, r.name); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = manifest_detail::append_text_field(body, manifest_detail::tag_path, r.path); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = manifest_detail::append_number_field(body, manifest_detail::tag_size, r.byte_size); !ok) {
            return std::unexpected(ok.error());
        }
        if (auto ok = manifest_detail::append_number_field(body, manifest_detail::tag_mtime, r.modified_at_ms); !ok) {
            return std::unexpected(ok.error());
        }
        if (version == WireVersion::v2 && r.note) {
            if (auto ok = manifest_detail::append_text_field(body, manifest_detail::tag_note, *r.note); !ok) {
                return std::unexpected(ok.error());
            }
        }
        if (body.size() > std::numeric_limits<std::uint32_t>::max()) {
            return std::unexpected(manifest_detail::err(Errc::limit_exceeded, out.size(), "record"));
        }
        if (auto ok = manifest_detail::append_checked(out, 4 + body.size()); !ok) {
            return std::unexpected(ok.error());
        }
        append_be<std::uint32_t>(out, static_cast<std::uint32_t>(body.size()));
        out.insert(out.end(), body.begin(), body.end());
    }
    return out;
}

inline std::expected<Manifest, DataError> decode_manifest(std::span<const std::byte> input)
{
    if (input.size() > max_package_bytes) {
        return std::unexpected(manifest_detail::err(Errc::limit_exceeded, 0, "package"));
    }
    if (input.size() < manifest_detail::header_size) {
        return std::unexpected(manifest_detail::err(Errc::incomplete_input, 0, "header"));
    }
    for (std::size_t i = 0; i != manifest_detail::magic.size(); ++i) {
        if (input[i] != static_cast<std::byte>(static_cast<unsigned char>(manifest_detail::magic[i]))) {
            return std::unexpected(manifest_detail::err(Errc::invalid_value, i, "magic"));
        }
    }

    std::size_t cursor = 4;
    auto major = read_be<std::uint16_t>(input, cursor);
    auto minor = read_be<std::uint16_t>(input, cursor);
    auto count = read_be<std::uint32_t>(input, cursor);
    if (!major || !minor || !count) {
        return std::unexpected(manifest_detail::err(Errc::incomplete_input, cursor, "header"));
    }
    if (*major != 1 || *minor == 0) {
        return std::unexpected(manifest_detail::err(Errc::unsupported_version, 4, "version"));
    }
    if (*count > max_records) {
        return std::unexpected(manifest_detail::err(Errc::limit_exceeded, 8, "records"));
    }

    Manifest manifest;
    manifest.records.reserve(*count);
    std::unordered_set<std::uint32_t> ids;
    ids.reserve(*count);

    for (std::uint32_t index = 0; index != *count; ++index) {
        const auto record_start = cursor;
        auto body_len = read_be<std::uint32_t>(input, cursor);
        if (!body_len) {
            return std::unexpected(body_len.error());
        }
        if (*body_len > max_package_bytes) {
            return std::unexpected(manifest_detail::err(Errc::limit_exceeded, record_start, "record"));
        }
        if (input.size() - cursor < *body_len) {
            return std::unexpected(manifest_detail::err(Errc::incomplete_input, record_start, "record"));
        }
        const auto record_end = cursor + *body_len;
        ResourceRecord r;
        bool seen_id = false;
        bool seen_name = false;
        bool seen_path = false;
        bool seen_size = false;
        bool seen_mtime = false;
        std::unordered_set<std::uint16_t> seen_unknown;

        while (cursor < record_end) {
            const auto field_start = cursor;
            auto tag = read_be<std::uint16_t>(input, cursor);
            auto len = read_be<std::uint32_t>(input, cursor);
            if (!tag || !len) {
                return std::unexpected(manifest_detail::err(Errc::incomplete_input, field_start, "field"));
            }
            if (*len > max_text_bytes && (*tag == manifest_detail::tag_name || *tag == manifest_detail::tag_path || *tag == manifest_detail::tag_note)) {
                return std::unexpected(manifest_detail::err(Errc::limit_exceeded, field_start, "text"));
            }
            if (record_end - cursor < *len) {
                return std::unexpected(manifest_detail::err(Errc::incomplete_input, field_start, "field"));
            }
            auto payload = input.subspan(cursor, *len);
            cursor += *len;

            auto duplicate = [&](bool& seen, std::string field) -> std::expected<void, DataError> {
                if (seen) {
                    return std::unexpected(manifest_detail::err(Errc::duplicate_field, field_start, std::move(field)));
                }
                seen = true;
                return {};
            };

            switch (*tag) {
            case manifest_detail::tag_id: {
                if (auto ok = duplicate(seen_id, "id"); !ok) {
                    return std::unexpected(ok.error());
                }
                auto value = manifest_detail::read_u32_payload(payload, field_start, "id");
                if (!value) {
                    return std::unexpected(value.error());
                }
                r.id = *value;
                break;
            }
            case manifest_detail::tag_name:
                if (auto ok = duplicate(seen_name, "name"); !ok) {
                    return std::unexpected(ok.error());
                }
                r.name = manifest_detail::bytes_to_string(payload);
                if (auto ok = manifest_detail::validate_text(r.name, "name", false, false); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            case manifest_detail::tag_path:
                if (auto ok = duplicate(seen_path, "path"); !ok) {
                    return std::unexpected(ok.error());
                }
                r.path = manifest_detail::bytes_to_string(payload);
                if (auto ok = manifest_detail::validate_text(r.path, "path", false, false); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                if (auto ok = validate_resource_path(r.path); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            case manifest_detail::tag_size: {
                if (auto ok = duplicate(seen_size, "size"); !ok) {
                    return std::unexpected(ok.error());
                }
                auto value = manifest_detail::read_u64_payload(payload, field_start, "size");
                if (!value) {
                    return std::unexpected(value.error());
                }
                r.byte_size = *value;
                break;
            }
            case manifest_detail::tag_mtime: {
                if (auto ok = duplicate(seen_mtime, "mtime"); !ok) {
                    return std::unexpected(ok.error());
                }
                auto value = manifest_detail::read_u64_payload(payload, field_start, "mtime");
                if (!value) {
                    return std::unexpected(value.error());
                }
                r.modified_at_ms = std::bit_cast<std::int64_t>(*value);
                break;
            }
            case manifest_detail::tag_note:
                if (*minor < 2) {
                    return std::unexpected(manifest_detail::err(Errc::unsupported_version, field_start, "note"));
                }
                if (r.note) {
                    return std::unexpected(manifest_detail::err(Errc::duplicate_field, field_start, "note"));
                }
                r.note = manifest_detail::bytes_to_string(payload);
                if (auto ok = manifest_detail::validate_text(*r.note, "note", true, false); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            default:
                if (!seen_unknown.insert(*tag).second) {
                    return std::unexpected(manifest_detail::err(Errc::duplicate_field, field_start, "unknown"));
                }
                break;
            }
        }
        if (!(seen_id && seen_name && seen_path && seen_size && seen_mtime)) {
            return std::unexpected(manifest_detail::err(Errc::missing_field, record_start, "record"));
        }
        if (r.id == 0) {
            return std::unexpected(manifest_detail::err(Errc::invalid_value, record_start, "id"));
        }
        if (!ids.insert(r.id).second) {
            return std::unexpected(manifest_detail::err(Errc::duplicate_field, record_start, "id"));
        }
        if (r.modified_at_ms < min_timestamp_ms || r.modified_at_ms > max_timestamp_ms) {
            return std::unexpected(manifest_detail::err(Errc::out_of_range, record_start, "mtime"));
        }
        manifest.records.push_back(std::move(r));
    }
    if (cursor != input.size()) {
        return std::unexpected(manifest_detail::err(Errc::trailing_data, cursor, "package"));
    }
    return manifest;
}
}
