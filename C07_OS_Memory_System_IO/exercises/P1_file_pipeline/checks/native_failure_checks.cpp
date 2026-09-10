#include <c07/file_pipeline_io.hpp>
#include <check.hpp>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <set>

int main() {
    std::vector<std::byte> input(8193);
    for (std::size_t i = 0; i < input.size(); ++i) input[i] = std::byte((i * 17 + 23) & 255);
    const auto path = std::filesystem::temp_directory_path() /
        ("c07-failure-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto written = c07::write_new_file(path, input);
    check(written.has_value(), "failure fixture created");
    auto failed = c07::read_file_chunks(path, c07::file_backend::completion, 1024, 4);
    if (!failed && failed.error() == std::make_error_code(std::errc::function_not_supported)) {
        std::filesystem::remove(path);
        std::cout << "SKIP: native completion capability unavailable\n";
        return 77;
    }
    check(!failed && failed.error() == std::make_error_code(std::errc::not_enough_memory), "injected allocation error returned after cleanup");
    check(c07::pipeline_detail::injected_failures == 1, "allocation failure was actually injected");
    check(c07::pipeline_detail::cleanup_retirements == 3, "three remaining target completions retired before unwind");
    auto recovered = c07::read_file_chunks(path, c07::file_backend::completion, 1024, 4);
    check(recovered.has_value(), "new request batch works after failure");
    std::set<std::uint64_t> ids;
    std::size_t total = 0;
    for (const auto& chunk : recovered->chunks) {
        check(chunk.request_id > 0 && chunk.request_id <= 9 && ids.insert(chunk.request_id).second, "each recovered request completed once");
        check(chunk.offset == (chunk.request_id - 1) * 1024 && chunk.bytes.size() <= input.size() - chunk.offset, "recovered interval valid");
        check(std::equal(chunk.bytes.begin(), chunk.bytes.end(), input.begin() + chunk.offset), "recovered bytes came from original file");
        total += chunk.bytes.size();
    }
    check(total == input.size() && ids.size() == 9, "recovered batch covers all bytes");
    check(std::filesystem::remove(path), "failure fixture removed");
    std::cout << "native allocation failure: three queued target obligations drained; subsequent batch passed\n";
}
