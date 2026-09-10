#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>

#include <c05/bytes.hpp>
#include <c05/paths.hpp>
#include <c05/model.hpp>
#include <c05/types.hpp>
#include <c05/utf.hpp>

namespace c05_l15 {
inline std::expected<c05::Manifest, c05::DataError> decode_manifest_v1(std::span<const std::byte> input)
{
    if (input.size() > c05::max_package_bytes || input.size() < 12) {
        return std::unexpected(c05::DataError{c05::Errc::incomplete_input, 0, c05::OffsetUnit::byte, "header"});
    }
    if (input[0] != std::byte{0x43} || input[1] != std::byte{0x30} || input[2] != std::byte{0x35} || input[3] != std::byte{0x4d}) {
        return std::unexpected(c05::DataError{c05::Errc::invalid_value, 0, c05::OffsetUnit::byte, "magic"});
    }
    std::size_t cursor = 4;
    auto major = c05::read_be<std::uint16_t>(input, cursor);
    auto minor = c05::read_be<std::uint16_t>(input, cursor);
    auto count = c05::read_be<std::uint32_t>(input, cursor);
    if (!major || !minor || !count || *major != 1 || *minor == 0 || *count > c05::max_records) {
        return std::unexpected(c05::DataError{c05::Errc::unsupported_version, 4, c05::OffsetUnit::byte, "version"});
    }

    c05::Manifest m;
    std::unordered_set<std::uint32_t> ids;
    for (std::uint32_t n = 0; n != *count; ++n) {
        const auto record_start = cursor;
        auto body_len = c05::read_be<std::uint32_t>(input, cursor);
        if (!body_len || input.size() - cursor < *body_len) {
            return std::unexpected(c05::DataError{c05::Errc::incomplete_input, record_start, c05::OffsetUnit::byte, "record"});
        }
        const auto end = cursor + *body_len;
        c05::ResourceRecord r;
        bool seen_id = false;
        bool seen_name = false;
        bool seen_path = false;
        bool seen_size = false;
        bool seen_mtime = false;
        std::unordered_set<std::uint16_t> unknown;
        while (cursor < end) {
            const auto field_start = cursor;
            auto tag = c05::read_be<std::uint16_t>(input, cursor);
            auto len = c05::read_be<std::uint32_t>(input, cursor);
            if (!tag || !len || end - cursor < *len) {
                return std::unexpected(c05::DataError{c05::Errc::incomplete_input, field_start, c05::OffsetUnit::byte, "field"});
            }
            auto payload = input.subspan(cursor, *len);
            cursor += *len;
            auto duplicate = [&](bool& seen, std::string field) -> std::expected<void, c05::DataError> {
                if (seen) {
                    return std::unexpected(c05::DataError{c05::Errc::duplicate_field, field_start, c05::OffsetUnit::byte, std::move(field)});
                }
                seen = true;
                return {};
            };
            auto as_string = [](std::span<const std::byte> bytes) {
                std::string s;
                for (auto b : bytes) {
                    s.push_back(static_cast<char>(std::to_integer<unsigned char>(b)));
                }
                return s;
            };
            switch (*tag) {
            case 1: {
                if (auto ok = duplicate(seen_id, "id"); !ok) {
                    return std::unexpected(ok.error());
                }
                if (payload.size() != 4) {
                    return std::unexpected(c05::DataError{c05::Errc::invalid_value, field_start, c05::OffsetUnit::byte, "id"});
                }
                std::size_t inner = 0;
                r.id = *c05::read_be<std::uint32_t>(payload, inner);
                break;
            }
            case 2:
                if (auto ok = duplicate(seen_name, "name"); !ok) {
                    return std::unexpected(ok.error());
                }
                r.name = as_string(payload);
                if (r.name.empty() || r.name.find('\0') != std::string::npos || !c05::validate_utf8(r.name)) {
                    return std::unexpected(c05::DataError{c05::Errc::invalid_value, field_start, c05::OffsetUnit::byte, "name"});
                }
                break;
            case 3:
                if (auto ok = duplicate(seen_path, "path"); !ok) {
                    return std::unexpected(ok.error());
                }
                r.path = as_string(payload);
                if (r.path.empty() || r.path.find('\0') != std::string::npos || !c05::validate_utf8(r.path)) {
                    return std::unexpected(c05::DataError{c05::Errc::invalid_value, field_start, c05::OffsetUnit::byte, "path"});
                }
                if (auto ok = c05::validate_resource_path(r.path); !ok) {
                    auto e = ok.error();
                    e.offset += field_start + 6;
                    return std::unexpected(e);
                }
                break;
            case 4: {
                if (auto ok = duplicate(seen_size, "size"); !ok) {
                    return std::unexpected(ok.error());
                }
                if (payload.size() != 8) {
                    return std::unexpected(c05::DataError{c05::Errc::invalid_value, field_start, c05::OffsetUnit::byte, "size"});
                }
                std::size_t inner = 0;
                r.byte_size = *c05::read_be<std::uint64_t>(payload, inner);
                break;
            }
            case 5: {
                if (auto ok = duplicate(seen_mtime, "mtime"); !ok) {
                    return std::unexpected(ok.error());
                }
                if (payload.size() != 8) {
                    return std::unexpected(c05::DataError{c05::Errc::invalid_value, field_start, c05::OffsetUnit::byte, "mtime"});
                }
                std::size_t inner = 0;
                auto raw = c05::read_be<std::uint64_t>(payload, inner);
                r.modified_at_ms = std::bit_cast<std::int64_t>(*raw);
                break;
            }
            default:
                if (!unknown.insert(*tag).second) {
                    return std::unexpected(c05::DataError{c05::Errc::duplicate_field, field_start, c05::OffsetUnit::byte, "unknown"});
                }
                break;
            }
        }
        if (!(seen_id && seen_name && seen_path && seen_size && seen_mtime)) {
            return std::unexpected(c05::DataError{c05::Errc::missing_field, record_start, c05::OffsetUnit::byte, "record"});
        }
        if (r.id == 0 || !ids.insert(r.id).second || r.modified_at_ms < c05::min_timestamp_ms || r.modified_at_ms > c05::max_timestamp_ms) {
            return std::unexpected(c05::DataError{c05::Errc::invalid_value, record_start, c05::OffsetUnit::byte, "record"});
        }
        m.records.push_back(std::move(r));
    }
    if (cursor != input.size()) {
        return std::unexpected(c05::DataError{c05::Errc::trailing_data, cursor, c05::OffsetUnit::byte, "package"});
    }
    return m;
}
}
