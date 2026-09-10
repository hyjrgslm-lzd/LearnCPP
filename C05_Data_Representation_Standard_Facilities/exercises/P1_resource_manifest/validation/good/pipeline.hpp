#pragma once
#include <contract.hpp>
#include <operations.hpp>
#include <c05/config.hpp>
#include <c05/manifest.hpp>

namespace c05_ex {
inline std::expected<PipelineResult, c05::DataError> run_pipeline(
    const c05::Manifest& source, std::string_view settings, const std::filesystem::path& directory) {
    const auto config = c05::parse_config(settings);
    if (!config) return std::unexpected(config.error());
    const auto valid = c05::validate_manifest(source);
    if (!valid) return std::unexpected(valid.error());
    const auto preview = c05_lab::render_report(source, config->display_zone);
    if (!preview) return std::unexpected(preview.error());
    const auto encoded = c05::encode_manifest(source);
    if (!encoded) return std::unexpected(encoded.error());
    const auto destination = directory / c05_lab::utf8_path(config->package_file);
    if (auto output = c05_lab::write_new(destination, *encoded); !output) return std::unexpected(output.error());
    auto input = c05_lab::read_bounded(destination);
    if (!input) return std::unexpected(input.error());
    auto value = c05::decode_manifest(*input);
    if (!value) return std::unexpected(value.error());
    auto text = c05_lab::render_report(*value, config->display_zone);
    if (!text) return std::unexpected(text.error());
    PipelineResult complete{std::move(*value), std::move(*text), destination};
    return complete;
}
}
