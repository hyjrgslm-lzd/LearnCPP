#include <pipeline.hpp>
#include <operations.hpp>
#include <check.hpp>
#include "../../fixtures/golden.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>

int main() {
    namespace fs = std::filesystem;
    std::vector<std::string> failures;
    const auto expect = [&](bool yes, std::string_view message) { if (!yes) failures.emplace_back(message); };
    fs::path scratch;
    std::error_code error;
    const auto temporary = fs::temp_directory_path(error);
    check(!error, "temporary directory available");
    auto parent = fs::canonical(temporary, error);
    check(!error, "temporary directory resolves to an existing absolute path");
    if (!parent.has_filename() && parent != parent.root_path()) parent = parent.parent_path();
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 64 && scratch.empty(); ++attempt) {
        const auto candidate = parent / ("c05-p1-" + std::to_string(tick) + "-" + std::to_string(attempt));
        error.clear();
        if (fs::create_directory(candidate, error)) scratch = candidate;
        else if (error) break;
    }
    check(!scratch.empty(), "create a new exclusively owned scratch directory");
    // Accumulate checks so bad/student exits happen only after owned cleanup.
    try {
        auto input = c05::fixtures::manifest_v2_empty_note();
        auto result = c05_ex::run_pipeline(input, "display_zone=UTC\n", scratch);
        expect(result.has_value(), "pipeline succeeds for valid input");
        const auto file = scratch / "manifest.c05m";
        expect(fs::is_regular_file(file), "creates the package file");
        if (result) {
            expect(result->manifest == input, "returns decoded owning manifest including empty note");
            expect(result->report.find("1 records") != std::string::npos && result->report.find("1970-01-01") != std::string::npos,
                "report carries decoded record count and timestamp");
            input.records[0].name = "changed";
            expect(result->manifest.records[0].name == "A", "result does not borrow input records");
        }
        if (fs::is_regular_file(file)) {
            const auto bytes = c05_lab::read_bounded(file);
            expect(bytes && std::ranges::equal(*bytes, std::as_bytes(std::span(c05::fixtures::v2_empty_note_octets))),
                "actual file matches independently hand-written v2 golden bytes");
        }
        auto varied = c05::fixtures::manifest_v1();
        varied.records.push_back({2, "资源", "assets/模型.bin", 123456789, -1, std::string("memo")});
        const auto alternate = c05_ex::run_pipeline(varied, "package_file=other.c05m\r\ndisplay_zone=UTC\r\n", scratch);
        expect(alternate && alternate->manifest == varied && alternate->report.find("123456789") != std::string::npos,
            "pipeline consumes varied unicode fields sizes and negative epoch");
        const auto bad_config = c05_ex::run_pipeline(varied, "package_file=bad.c05m\nunknown=x\n", scratch);
        expect(!bad_config && !fs::exists(scratch / "bad.c05m"), "invalid config fails before any output");
        const auto empty_bad_zone = c05_ex::run_pipeline(c05::Manifest{},
            "package_file=empty-zone.c05m\ndisplay_zone=Not/A_Real_Zone\n", scratch);
        expect(!empty_bad_zone && !fs::exists(scratch / "empty-zone.c05m"), "empty manifest validates configured timezone");
        varied.records[1].id = 1;
        const auto bad_value = c05_ex::run_pipeline(varied, "package_file=invalid.c05m", scratch);
        expect(!bad_value && !fs::exists(scratch / "invalid.c05m"), "invalid manifest fails before any output");
        const auto preserved = scratch / "keep.c05m";
        { std::ofstream sentinel(preserved, std::ios::binary); sentinel << "keep"; expect(bool(sentinel), "create preservation fixture"); }
        const auto collision = c05_ex::run_pipeline(c05::fixtures::manifest_v1(), "package_file=keep.c05m", scratch);
        expect(!collision && collision.error().code == c05::Errc::io_error, "existing output is rejected");
        { std::ifstream actual(preserved, std::ios::binary); std::string contents{std::istreambuf_iterator<char>(actual), {}};
          expect(contents == "keep", "existing output bytes are preserved"); }
        const auto missing = c05_ex::run_pipeline(c05::fixtures::manifest_v1(), "", scratch / "absent");
        expect(!missing && missing.error().code == c05::Errc::io_error, "missing output directory reports io error");
        auto bytes = c05_lab::read_bounded(scratch / "never-created");
        expect(!bytes && bytes.error().code == c05::Errc::io_error, "bounded input reports missing file");
    } catch (const std::exception& exception) {
        failures.emplace_back(std::string("unexpected exception: ") + exception.what());
    }
    // The only recursive cleanup target was created successfully above; it is
    // an absolute direct child of the captured temp parent, never a resource path.
    const bool owned = scratch.is_absolute() && scratch.parent_path() == parent;
    if (!owned) std::cerr << "scratch parent check: expected " << parent << " actual " << scratch.parent_path() << '\n';
    if (owned) fs::remove_all(scratch, error);
    // Cleanup failure must outrank an expected bad/student diagnostic.
    check(owned, "cleanup path remains owned temp child");
    check(!error && !fs::exists(scratch), "owned temporary files are cleaned");
    for (const auto& failure : failures) check(false, failure);
    std::cout << "resource manifest pipeline checked\n";
}
