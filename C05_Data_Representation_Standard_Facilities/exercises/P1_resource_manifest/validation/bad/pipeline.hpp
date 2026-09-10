#pragma once
#include <contract.hpp>
#include <operations.hpp>
#include <c05/config.hpp>
#include <c05/manifest.hpp>
namespace c05_ex {
inline std::expected<PipelineResult, c05::DataError> run_pipeline(
    const c05::Manifest& source, std::string_view settings, const std::filesystem::path& directory) {
    auto config = c05::parse_config(settings);
    if (!config) return std::unexpected(config.error());
    auto valid = c05::validate_manifest(source);
    if (!valid) return std::unexpected(valid.error());
    auto report = c05_lab::render_report(source, config->display_zone);
    if (!report) return std::unexpected(report.error());
    // Deliberate shortcut: reporting success without writing or reading bytes.
    return PipelineResult{source, *report, directory / c05_lab::utf8_path(config->package_file)};
}
}
