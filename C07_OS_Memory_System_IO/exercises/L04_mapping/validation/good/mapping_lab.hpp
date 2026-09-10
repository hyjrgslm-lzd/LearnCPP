#ifndef C07_L04_MAPPING_LAB_HPP
#define C07_L04_MAPPING_LAB_HPP

#include <c07/memory.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <limits>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace c07_l04 {

struct window_plan {
    std::uint64_t aligned_offset{};
    std::size_t delta{};
    std::size_t mapped_length{};
    std::size_t visible_length{};
};

enum class write_mapping_mode {
    shared,
    private_copy
};

inline std::expected<window_plan, std::error_code> plan_readonly_window(
    std::uint64_t file_size, std::uint64_t offset, std::size_t length, std::size_t granularity) {
    if (granularity == 0 || length == 0 || offset >= file_size) {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }
    const std::uint64_t aligned = offset / granularity * granularity;
    const auto delta = static_cast<std::size_t>(offset - aligned);
    const auto visible = static_cast<std::size_t>(
        std::min<std::uint64_t>(file_size - offset, static_cast<std::uint64_t>(length)));
    if (visible > std::numeric_limits<std::size_t>::max() - delta) {
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    }
    return window_plan{aligned, delta, delta + visible, visible};
}

inline std::expected<c07::readonly_mapping, std::error_code> map_window(
    const std::filesystem::path& path, std::uint64_t offset, std::size_t length) {
    return c07::map_readonly(path, offset, length);
}

inline std::expected<void, std::error_code> write_first_byte(
    const std::filesystem::path& path, write_mapping_mode mode, char value) {
#ifdef _WIN32
    c07::unique_handle f{::CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (!f) return std::unexpected(c07::last_error_code());
    c07::unique_handle m{::CreateFileMappingW(f.get(), nullptr,
        mode == write_mapping_mode::shared ? PAGE_READWRITE : PAGE_WRITECOPY, 0, 0, nullptr)};
    if (!m) return std::unexpected(c07::last_error_code());
    void* v = ::MapViewOfFile(m.get(),
        mode == write_mapping_mode::shared ? FILE_MAP_WRITE : FILE_MAP_COPY, 0, 0, 1);
    if (!v) return std::unexpected(c07::last_error_code());
    static_cast<char*>(v)[0] = value;
    std::error_code ec;
    if (mode == write_mapping_mode::shared && !::FlushViewOfFile(v, 1)) ec = c07::last_error_code();
    (void)::UnmapViewOfFile(v);
    if (ec) return std::unexpected(ec);
    return {};
#else
    int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) return std::unexpected(c07::last_error_code());
    void* v = ::mmap(nullptr, 1, PROT_READ | PROT_WRITE,
        mode == write_mapping_mode::shared ? MAP_SHARED : MAP_PRIVATE, fd, 0);
    int saved = errno;
    (void)::close(fd);
    if (v == MAP_FAILED) {
        errno = saved;
        return std::unexpected(c07::last_error_code());
    }
    static_cast<char*>(v)[0] = value;
    std::error_code ec;
    if (mode == write_mapping_mode::shared && ::msync(v, 1, MS_SYNC) != 0) ec = c07::last_error_code();
    (void)::munmap(v, 1);
    if (ec) return std::unexpected(ec);
    return {};
#endif
}

} // namespace c07_l04

#endif
