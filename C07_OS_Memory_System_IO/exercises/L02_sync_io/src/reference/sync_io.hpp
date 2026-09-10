#pragma once
#include <c07/os.hpp>
#include <algorithm>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <system_error>
#include <vector>

namespace c07_l02 {
inline std::expected<std::uintmax_t, std::error_code> copy_file_exact(
    const std::filesystem::path& source, const std::filesystem::path& target, std::size_t chunk_size) {
    if (chunk_size == 0) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    auto in = c07::open_existing_file(source);
    if (!in) return std::unexpected(in.error());
    auto out = c07::create_new_file(target);
    if (!out) return std::unexpected(out.error());

    std::vector<std::byte> buffer(std::min<std::size_t>(chunk_size, 64 * 1024));
    std::uintmax_t copied = 0;
    for (;;) {
        auto got = c07::read_some(in->get(), buffer);
        if (!got) return std::unexpected(got.error());
        if (*got == 0) return copied;
        if (auto written = c07::write_all(out->get(), std::span<const std::byte>{buffer.data(), *got}); !written) {
            return std::unexpected(written.error());
        }
        copied += *got;
    }
}
} // namespace c07_l02
