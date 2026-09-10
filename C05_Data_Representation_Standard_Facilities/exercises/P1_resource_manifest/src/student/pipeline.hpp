#pragma once
#include <contract.hpp>
namespace c05_ex {
inline std::expected<PipelineResult, c05::DataError> run_pipeline(
    const c05::Manifest&, std::string_view, const std::filesystem::path&) {
    // Implement the pipeline in the README. The provided lower-level operations
    // are permitted; importing another pipeline implementation is not.
    return std::unexpected(c05::DataError{c05::Errc::not_implemented, 0, c05::OffsetUnit::byte, "pipeline"});
}
}
