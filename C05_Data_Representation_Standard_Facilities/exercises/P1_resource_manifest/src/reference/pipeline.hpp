#pragma once
#include <contract.hpp>
#include <operations.hpp>
#include <c05/config.hpp>
#include <c05/manifest.hpp>

namespace c05_ex {
inline std::expected<PipelineResult, c05::DataError> run_pipeline(
    const c05::Manifest& source, std::string_view config_text, const std::filesystem::path& scratch) {
    auto config = c05::parse_config(config_text);
    if (!config) return std::unexpected(config.error());
    auto bytes = c05::encode_manifest(source, c05::WireVersion::v2);
    if (!bytes) return std::unexpected(bytes.error());
    // Reject display failures before creating a file, too.
    auto report = c05_lab::render_report(source, config->display_zone);
    if (!report) return std::unexpected(report.error());
    const auto file = scratch / c05_lab::utf8_path(config->package_file);
    auto written = c05_lab::write_new(file, *bytes);
    if (!written) return std::unexpected(written.error());
    auto actual_bytes = c05_lab::read_bounded(file);
    if (!actual_bytes) return std::unexpected(actual_bytes.error());
    auto decoded = c05::decode_manifest(*actual_bytes);
    if (!decoded) return std::unexpected(decoded.error());
    auto actual_report = c05_lab::render_report(*decoded, config->display_zone);
    if (!actual_report) return std::unexpected(actual_report.error());
    return PipelineResult{std::move(*decoded), std::move(*actual_report), file};
}
}
