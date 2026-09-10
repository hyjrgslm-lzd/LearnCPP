#pragma once
#include <c05/model.hpp>
#include <filesystem>
#include <string_view>

namespace c05_ex {
struct PipelineResult {
    c05::Manifest manifest;
    std::string report;
    std::filesystem::path package_file;
};
// run_pipeline(Manifest, UTF-8 config, caller-owned existing scratch directory)
// returns owning results. A write failure can leave a new partial file in that
// scratch directory; the caller owns its cleanup. No existing file is replaced.
}
