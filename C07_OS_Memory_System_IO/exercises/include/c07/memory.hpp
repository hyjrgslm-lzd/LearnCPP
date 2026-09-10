#ifndef C07_OS_MEMORY_SYSTEM_IO_MEMORY_HPP
#define C07_OS_MEMORY_SYSTEM_IO_MEMORY_HPP

#include <c07/os.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <limits>
#include <span>
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace c07 {

struct page_info {
    std::size_t page_size{};
    std::size_t allocation_granularity{};
};

enum class page_access {
    no_access,
    read_only,
    read_write
};

inline page_info query_page_info() noexcept {
#ifdef _WIN32
    SYSTEM_INFO info{};
    ::GetSystemInfo(&info);
    return {info.dwPageSize, info.dwAllocationGranularity};
#else
    const long page = ::sysconf(_SC_PAGESIZE);
    const auto size = page > 0 ? static_cast<std::size_t>(page) : std::size_t{};
    return {size, size};
#endif
}

namespace detail {

inline bool range_overflows(std::size_t offset, std::size_t length, std::size_t limit) noexcept {
    return offset > limit || length > limit - offset;
}

#ifdef _WIN32
inline DWORD protect_flag(page_access access) noexcept {
    switch (access) {
    case page_access::no_access:
        return PAGE_NOACCESS;
    case page_access::read_only:
        return PAGE_READONLY;
    case page_access::read_write:
        return PAGE_READWRITE;
    }
    return PAGE_NOACCESS;
}
#else
inline int protect_flag(page_access access) noexcept {
    switch (access) {
    case page_access::no_access:
        return PROT_NONE;
    case page_access::read_only:
        return PROT_READ;
    case page_access::read_write:
        return PROT_READ | PROT_WRITE;
    }
    return PROT_NONE;
}
#endif

} // namespace detail

class virtual_region {
public:
    virtual_region() noexcept = default;
    virtual_region(const virtual_region&) = delete;
    virtual_region& operator=(const virtual_region&) = delete;

    virtual_region(virtual_region&& other) noexcept
        : base_(std::exchange(other.base_, nullptr)), size_(std::exchange(other.size_, 0)) {}

    virtual_region& operator=(virtual_region&& other) noexcept {
        if (this != &other) {
            release();
            base_ = std::exchange(other.base_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    ~virtual_region() { release(); }

    [[nodiscard]] static std::expected<virtual_region, std::error_code> reserve(std::size_t bytes) {
        if (bytes == 0) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
#ifdef _WIN32
        void* base = ::VirtualAlloc(nullptr, bytes, MEM_RESERVE, PAGE_NOACCESS);
        if (base == nullptr) return std::unexpected(last_error_code());
#else
        void* base = ::mmap(nullptr, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED) return std::unexpected(last_error_code());
#endif
        return virtual_region(base, bytes);
    }

    [[nodiscard]] void* data() const noexcept { return base_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    explicit operator bool() const noexcept { return base_ != nullptr; }

    [[nodiscard]] std::span<std::byte> bytes(std::size_t offset, std::size_t length) const noexcept {
        if (!base_ || detail::range_overflows(offset, length, size_)) return {};
        return {static_cast<std::byte*>(base_) + offset, length};
    }

    [[nodiscard]] std::expected<void, std::error_code> commit(
        std::size_t offset, std::size_t length, page_access access = page_access::read_write) {
        if (!base_ || length == 0 || detail::range_overflows(offset, length, size_)) {
            return std::unexpected(std::make_error_code(std::errc::invalid_argument));
        }
#ifdef _WIN32
        if (::VirtualAlloc(static_cast<std::byte*>(base_) + offset, length, MEM_COMMIT,
                detail::protect_flag(access)) == nullptr) {
            return std::unexpected(last_error_code());
        }
#else
        if (::mprotect(static_cast<std::byte*>(base_) + offset, length, detail::protect_flag(access)) != 0) {
            return std::unexpected(last_error_code());
        }
#endif
        return {};
    }

    [[nodiscard]] std::expected<void, std::error_code> protect(
        std::size_t offset, std::size_t length, page_access access) {
        if (!base_ || length == 0 || detail::range_overflows(offset, length, size_)) {
            return std::unexpected(std::make_error_code(std::errc::invalid_argument));
        }
#ifdef _WIN32
        DWORD old_protect = 0;
        if (!::VirtualProtect(static_cast<std::byte*>(base_) + offset, length,
                detail::protect_flag(access), &old_protect)) {
            return std::unexpected(last_error_code());
        }
#else
        if (::mprotect(static_cast<std::byte*>(base_) + offset, length, detail::protect_flag(access)) != 0) {
            return std::unexpected(last_error_code());
        }
#endif
        return {};
    }

    [[nodiscard]] std::expected<void, std::error_code> decommit(std::size_t offset, std::size_t length) {
        if (!base_ || length == 0 || detail::range_overflows(offset, length, size_)) {
            return std::unexpected(std::make_error_code(std::errc::invalid_argument));
        }
#ifdef _WIN32
        if (!::VirtualFree(static_cast<std::byte*>(base_) + offset, length, MEM_DECOMMIT)) {
            return std::unexpected(last_error_code());
        }
#else
        if (::mprotect(static_cast<std::byte*>(base_) + offset, length, PROT_NONE) != 0) {
            return std::unexpected(last_error_code());
        }
#endif
        return {};
    }

    void release() noexcept {
        if (!base_) return;
#ifdef _WIN32
        (void)::VirtualFree(base_, 0, MEM_RELEASE);
#else
        (void)::munmap(base_, size_);
#endif
        base_ = nullptr;
        size_ = 0;
    }

private:
    virtual_region(void* base, std::size_t size) noexcept : base_(base), size_(size) {}

    void* base_ = nullptr;
    std::size_t size_ = 0;
};

class readonly_mapping {
public:
    readonly_mapping() noexcept = default;
    readonly_mapping(const readonly_mapping&) = delete;
    readonly_mapping& operator=(const readonly_mapping&) = delete;

    readonly_mapping(readonly_mapping&& other) noexcept
        : file_(std::move(other.file_))
#ifdef _WIN32
        , mapping_(std::move(other.mapping_))
#endif
        , base_(std::exchange(other.base_, nullptr))
        , mapped_size_(std::exchange(other.mapped_size_, 0))
        , delta_(std::exchange(other.delta_, 0))
        , size_(std::exchange(other.size_, 0)) {}

    readonly_mapping& operator=(readonly_mapping&& other) noexcept {
        if (this != &other) {
            reset();
            file_ = std::move(other.file_);
#ifdef _WIN32
            mapping_ = std::move(other.mapping_);
#endif
            base_ = std::exchange(other.base_, nullptr);
            mapped_size_ = std::exchange(other.mapped_size_, 0);
            delta_ = std::exchange(other.delta_, 0);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    ~readonly_mapping() { reset(); }

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
        if (!base_) return {};
        return {static_cast<const std::byte*>(base_) + delta_, size_};
    }

    void reset() noexcept {
        if (base_) {
#ifdef _WIN32
            (void)::UnmapViewOfFile(base_);
#else
            (void)::munmap(base_, mapped_size_);
#endif
            base_ = nullptr;
        }
        mapped_size_ = 0;
        delta_ = 0;
        size_ = 0;
#ifdef _WIN32
        mapping_.reset();
#endif
        file_.reset();
    }

private:
    friend std::expected<readonly_mapping, std::error_code> map_readonly(
        const std::filesystem::path&, std::uint64_t, std::size_t);

    unique_file file_{};
#ifdef _WIN32
    unique_handle mapping_{};
#endif
    void* base_ = nullptr;
    std::size_t mapped_size_ = 0;
    std::size_t delta_ = 0;
    std::size_t size_ = 0;
};

inline std::expected<readonly_mapping, std::error_code> map_readonly(
    const std::filesystem::path& path, std::uint64_t offset, std::size_t length) {
    if (length == 0) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    auto file = open_existing_file(path);
    if (!file) return std::unexpected(file.error());

    std::uint64_t file_size = 0;
#ifdef _WIN32
    LARGE_INTEGER size{};
    if (!::GetFileSizeEx(file->get(), &size)) return std::unexpected(last_error_code());
    file_size = static_cast<std::uint64_t>(size.QuadPart);
#else
    struct stat st {};
    if (::fstat(file->get(), &st) != 0) return std::unexpected(last_error_code());
    if (st.st_size < 0) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    file_size = static_cast<std::uint64_t>(st.st_size);
#endif
    if (offset >= file_size) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    const std::uint64_t available = file_size - offset;
    const std::size_t wanted = static_cast<std::size_t>(
        std::min<std::uint64_t>(available, static_cast<std::uint64_t>(length)));
    const auto info = query_page_info();
    const std::uint64_t granularity = info.allocation_granularity;
    if (granularity == 0) return std::unexpected(std::make_error_code(std::errc::not_supported));
    const std::uint64_t aligned_offset = offset - (offset % granularity);
    const std::size_t delta = static_cast<std::size_t>(offset - aligned_offset);
    if (wanted > std::numeric_limits<std::size_t>::max() - delta) {
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    }
    const std::size_t mapped_size = wanted + delta;

    readonly_mapping result;
    result.file_ = std::move(*file);
    result.delta_ = delta;
    result.size_ = wanted;
    result.mapped_size_ = mapped_size;

#ifdef _WIN32
    result.mapping_.reset(::CreateFileMappingW(result.file_.get(), nullptr, PAGE_READONLY, 0, 0, nullptr));
    if (!result.mapping_) return std::unexpected(last_error_code());
    LARGE_INTEGER where{};
    where.QuadPart = static_cast<LONGLONG>(aligned_offset);
    result.base_ = ::MapViewOfFile(result.mapping_.get(), FILE_MAP_READ,
        static_cast<DWORD>(where.HighPart), static_cast<DWORD>(where.LowPart), mapped_size);
    if (!result.base_) return std::unexpected(last_error_code());
#else
    result.base_ = ::mmap(nullptr, mapped_size, PROT_READ, MAP_SHARED,
        result.file_.get(), static_cast<off_t>(aligned_offset));
    if (result.base_ == MAP_FAILED) {
        result.base_ = nullptr;
        return std::unexpected(last_error_code());
    }
#endif
    return result;
}

} // namespace c07

#endif
