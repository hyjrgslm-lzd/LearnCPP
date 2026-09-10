#pragma once
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>
#include <vector>

namespace c07 {
inline constexpr std::size_t max_pipeline_bytes = 64 * 1024 * 1024;
inline constexpr std::size_t max_pipeline_chunks = 65536;
enum class file_backend { buffered, mapped, completion };
struct completed_chunk {
    std::uint64_t request_id{};
    std::size_t offset{};
    std::vector<std::byte> bytes;
};
struct read_metrics {
    std::size_t read_calls{};             // Calls at the instrumented API boundary, not all kernel activity.
    std::size_t completions{};
    std::size_t peak_in_flight{};         // Deferred completion requests only.
    std::size_t application_copy_bytes{}; // Explicit materialization copies, excludes initialization/kernel copies.
    double setup_seconds{};
    double read_seconds{};               // Includes completed-chunk materialization.
};
struct chunk_batch {
    std::size_t total_bytes{};
    std::vector<completed_chunk> chunks;
    read_metrics metrics;
};
// Defined in file_pipeline_io.hpp. The source is a stable local regular file.
std::expected<chunk_batch, std::error_code> read_file_chunks(const std::filesystem::path&,
    file_backend, std::size_t chunk_size = 65536, std::size_t in_flight = 4);
std::expected<void, std::error_code> write_new_file(const std::filesystem::path&,
    std::span<const std::byte>);
} // namespace c07
