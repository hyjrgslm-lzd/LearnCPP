#pragma once
#include <cstddef>
#include <expected>
#include <span>
#include <vector>

#include <c05/model.hpp>
#include <c05/types.hpp>

namespace c05_ex {
inline std::expected<void, c05::DataError> validate_manifest(const c05::Manifest&)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "manifest"});
}
inline std::expected<std::vector<std::byte>, c05::DataError> encode_manifest(
    const c05::Manifest&, c05::WireVersion = c05::WireVersion::v2)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "manifest"});
}
inline std::expected<c05::Manifest, c05::DataError> decode_manifest(std::span<const std::byte>)
{
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "manifest"});
}
}
