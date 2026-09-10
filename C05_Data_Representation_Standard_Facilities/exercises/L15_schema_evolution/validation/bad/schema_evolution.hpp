#pragma once
#include <cstddef>
#include <expected>
#include <span>
#include <vector>

#include <c05/manifest.hpp>
#include <c05/model.hpp>
#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<void, c05::DataError> validate_manifest(const c05::Manifest& m)
{
    return c05::validate_manifest(m);
}
inline std::expected<c05::Manifest, c05::DataError> decode_manifest(std::span<const std::byte> input)
{
    return c05::decode_manifest(input);
}
inline std::expected<std::vector<std::byte>, c05::DataError> encode_manifest(
    const c05::Manifest& m, c05::WireVersion version = c05::WireVersion::v2)
{
    auto copy = m;
    if (version == c05::WireVersion::v1) {
        for (auto& r : copy.records) {
            r.note.reset();
        }
    }
    return c05::encode_manifest(copy, version);
}
}
