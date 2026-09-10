#ifndef C07_OS_MEMORY_SYSTEM_IO_OS_HPP
#define C07_OS_MEMORY_SYSTEM_IO_OS_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <algorithm>
#include <limits>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>

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
#include <unistd.h>
#endif

namespace c07 {

#ifdef _WIN32
using native_handle = HANDLE;
inline const native_handle invalid_native_handle = INVALID_HANDLE_VALUE;

inline std::error_code last_error_code() noexcept {
    return {static_cast<int>(::GetLastError()), std::system_category()};
}

class unique_handle {
public:
    unique_handle() noexcept = default;
    explicit unique_handle(native_handle handle) noexcept : handle_(handle) {}
    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;
    unique_handle(unique_handle&& other) noexcept : handle_(std::exchange(other.handle_, invalid_native_handle)) {}
    unique_handle& operator=(unique_handle&& other) noexcept {
        if (this != &other) reset(std::exchange(other.handle_, invalid_native_handle));
        return *this;
    }
    ~unique_handle() { reset(); }

    [[nodiscard]] native_handle get() const noexcept { return handle_; }
    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }
    explicit operator bool() const noexcept { return valid(); }
    native_handle release() noexcept { return std::exchange(handle_, invalid_native_handle); }
    void reset(native_handle next = invalid_native_handle) noexcept {
        if (valid()) ::CloseHandle(handle_);
        handle_ = next;
    }

private:
    native_handle handle_ = invalid_native_handle;
};

using unique_file = unique_handle;

inline std::expected<unique_file, std::error_code> create_new_file(const std::filesystem::path& path) {
    unique_file file{::CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (!file) return std::unexpected(last_error_code());
    return file;
}

inline std::expected<unique_file, std::error_code> open_existing_file(const std::filesystem::path& path) {
    unique_file file{::CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (!file) return std::unexpected(last_error_code());
    return file;
}

inline std::expected<std::size_t, std::error_code> read_some(native_handle file, std::span<std::byte> buffer) {
    DWORD transferred = 0;
    const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(buffer.size(), UINT32_MAX));
    if (!::ReadFile(file, buffer.data(), requested, &transferred, nullptr)) {
        return std::unexpected(last_error_code());
    }
    return static_cast<std::size_t>(transferred);
}

inline std::expected<void, std::error_code> write_all(native_handle file, std::span<const std::byte> bytes) {
    while (!bytes.empty()) {
        DWORD transferred = 0;
        const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(bytes.size(), UINT32_MAX));
        if (!::WriteFile(file, bytes.data(), requested, &transferred, nullptr)) {
            return std::unexpected(last_error_code());
        }
        if (transferred == 0) return std::unexpected(std::make_error_code(std::errc::io_error));
        bytes = bytes.subspan(transferred);
    }
    return {};
}

#else
using native_handle = int;
inline constexpr native_handle invalid_native_handle = -1;

inline std::error_code last_error_code() noexcept {
    return {errno, std::generic_category()};
}

class unique_fd {
public:
    unique_fd() noexcept = default;
    explicit unique_fd(native_handle fd) noexcept : fd_(fd) {}
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;
    unique_fd(unique_fd&& other) noexcept : fd_(std::exchange(other.fd_, invalid_native_handle)) {}
    unique_fd& operator=(unique_fd&& other) noexcept {
        if (this != &other) reset(std::exchange(other.fd_, invalid_native_handle));
        return *this;
    }
    ~unique_fd() { reset(); }

    [[nodiscard]] native_handle get() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }
    explicit operator bool() const noexcept { return valid(); }
    native_handle release() noexcept { return std::exchange(fd_, invalid_native_handle); }
    void reset(native_handle next = invalid_native_handle) noexcept {
        if (valid()) (void)::close(fd_);
        fd_ = next;
    }

private:
    native_handle fd_ = invalid_native_handle;
};

using unique_file = unique_fd;

inline std::expected<unique_file, std::error_code> create_new_file(const std::filesystem::path& path) {
    for (;;) {
        int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0) return unique_file{fd};
        if (errno != EINTR) return std::unexpected(last_error_code());
    }
}

inline std::expected<unique_file, std::error_code> open_existing_file(const std::filesystem::path& path) {
    for (;;) {
        int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd >= 0) return unique_file{fd};
        if (errno != EINTR) return std::unexpected(last_error_code());
    }
}

inline std::expected<std::size_t, std::error_code> read_some(native_handle fd, std::span<std::byte> buffer) {
    const auto limit = static_cast<std::size_t>(std::numeric_limits<ssize_t>::max());
    buffer = buffer.first(std::min(buffer.size(), limit));
    for (;;) {
        const ssize_t n = ::read(fd, buffer.data(), buffer.size());
        if (n >= 0) return static_cast<std::size_t>(n);
        if (errno != EINTR) return std::unexpected(last_error_code());
    }
}

inline std::expected<void, std::error_code> write_all(native_handle fd, std::span<const std::byte> bytes) {
    const auto limit = static_cast<std::size_t>(std::numeric_limits<ssize_t>::max());
    while (!bytes.empty()) {
        const auto chunk = bytes.first(std::min(bytes.size(), limit));
        const ssize_t n = ::write(fd, chunk.data(), chunk.size());
        if (n > 0) {
            bytes = bytes.subspan(static_cast<std::size_t>(n));
            continue;
        }
        if (n == -1 && errno == EINTR) continue;
        if (n == -1) return std::unexpected(last_error_code());
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }
    return {};
}
#endif

inline std::span<const std::byte> bytes_of(std::span<const char> text) noexcept {
    return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

inline std::span<const std::byte> bytes_of(std::string_view text) noexcept {
    return bytes_of(std::span<const char>{text.data(), text.size()});
}

} // namespace c07

#endif
