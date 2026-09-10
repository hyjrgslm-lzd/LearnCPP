#pragma once
#include "contract.hpp"
#include <c05/time.hpp>
#include <fstream>
#include <format>
#include <limits>
#include <span>
#include <utility>

namespace c05_lab {
inline std::filesystem::path utf8_path(std::string_view text) {
    std::u8string units;
    for (const unsigned char byte : text) units.push_back(static_cast<char8_t>(byte));
    return std::filesystem::path(units);
}
inline std::expected<void, c05::DataError> write_new(
    const std::filesystem::path& file, std::span<const std::byte> bytes) {
    if (bytes.size() > c05::max_package_bytes || !std::in_range<std::streamsize>(bytes.size()))
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, 0, c05::OffsetUnit::byte, "output"});
    std::ofstream stream(file, std::ios::binary | std::ios::out | std::ios::noreplace);
    if (!stream.is_open())
        return std::unexpected(c05::DataError{c05::Errc::io_error, 0, c05::OffsetUnit::byte, "create output"});
    if (!bytes.empty()) stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.flush();
    if (!stream)
        return std::unexpected(c05::DataError{c05::Errc::io_error, 0, c05::OffsetUnit::byte, "write output"});
    stream.close();
    if (!stream)
        return std::unexpected(c05::DataError{c05::Errc::io_error, 0, c05::OffsetUnit::byte, "close output"});
    return {};
}
inline std::expected<std::vector<std::byte>, c05::DataError> read_bounded(const std::filesystem::path& file) {
    std::error_code error;
    const auto size = std::filesystem::file_size(file, error);
    if (error)
        return std::unexpected(c05::DataError{c05::Errc::io_error, 0, c05::OffsetUnit::byte, "file size"});
    if (size > c05::max_package_bytes || !std::in_range<std::streamsize>(size) || !std::in_range<std::size_t>(size))
        return std::unexpected(c05::DataError{c05::Errc::limit_exceeded, 0, c05::OffsetUnit::byte, "input"});
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        return std::unexpected(c05::DataError{c05::Errc::io_error, 0, c05::OffsetUnit::byte, "open input"});
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    if (!stream)
        return std::unexpected(c05::DataError{c05::Errc::io_error, 0, c05::OffsetUnit::byte, "short read"});
    char extra{};
    if (stream.get(extra) || stream.bad())
        return std::unexpected(c05::DataError{c05::Errc::io_error, bytes.size(), c05::OffsetUnit::byte, "changed input"});
    return bytes;
}
inline std::expected<std::string, c05::DataError> render_report(
    const c05::Manifest& manifest, std::string_view zone) {
    if (manifest.records.empty()) {
        auto valid_zone = c05::format_timestamp(0, zone);
        if (!valid_zone) return std::unexpected(valid_zone.error());
    }
    std::string report = std::format("C05 manifest: {} records\n", manifest.records.size());
    for (const auto& record : manifest.records) {
        auto stamp = c05::format_timestamp(record.modified_at_ms, zone);
        if (!stamp) return std::unexpected(stamp.error());
        report += std::format("id={} name={:?} path={:?} bytes={} modified={}\n",
            record.id, record.name, record.path, record.byte_size, *stamp);
    }
    return report;
}
}
