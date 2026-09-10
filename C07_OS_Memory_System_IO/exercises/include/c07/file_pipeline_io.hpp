#pragma once
#include <c07/file_pipeline_types.hpp>
#include <c07/completion_io.hpp>
#include <c07/memory.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <new>
#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace c07::pipeline_detail {
using clock = std::chrono::steady_clock;
inline double seconds(clock::time_point from) { return std::chrono::duration<double>(clock::now() - from).count(); }
inline std::error_code invalid() { return std::make_error_code(std::errc::invalid_argument); }
#ifdef C07_PIPELINE_FAILURE_TEST
inline bool inject_materialization_failure = true;
inline std::size_t injected_failures = 0;
inline std::size_t cleanup_retirements = 0;
#endif

inline std::expected<unique_file, std::error_code> open_source(const std::filesystem::path& path, file_backend mode) {
#ifdef _WIN32
    const DWORD flags = FILE_ATTRIBUTE_NORMAL | (mode == file_backend::completion ? FILE_FLAG_OVERLAPPED : 0);
    unique_file file{::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, flags, nullptr)};
    if (!file) return std::unexpected(last_error_code());
#else
    (void)mode;
    // O_NONBLOCK prevents an unexpected FIFO from blocking before fstat rejects it.
    int descriptor;
    do { descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK); }
    while (descriptor < 0 && errno == EINTR);
    if (descriptor < 0) return std::unexpected(last_error_code());
    unique_file file{descriptor};
#endif
    return file;
}

inline std::expected<std::size_t, std::error_code> source_size(native_handle file) {
#ifdef _WIN32
    if (::GetFileType(file) != FILE_TYPE_DISK) return std::unexpected(invalid());
    BY_HANDLE_FILE_INFORMATION info{};
    LARGE_INTEGER size{};
    if (!::GetFileInformationByHandle(file, &info) || !::GetFileSizeEx(file, &size)) return std::unexpected(last_error_code());
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || size.QuadPart < 0) return std::unexpected(invalid());
    const auto value = static_cast<std::uint64_t>(size.QuadPart);
#else
    struct stat info{};
    if (::fstat(file, &info) != 0) return std::unexpected(last_error_code());
    if (!S_ISREG(info.st_mode) || info.st_size < 0) return std::unexpected(invalid());
    const auto value = static_cast<std::uint64_t>(info.st_size);
#endif
    if (value > max_pipeline_bytes) return std::unexpected(std::make_error_code(std::errc::file_too_large));
    return static_cast<std::size_t>(value);
}

struct slot {
    operation_context context;
    std::size_t offset{};
    std::size_t requested{};
    bool active{};
};

inline std::size_t outstanding(std::span<const slot> slots) noexcept {
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [](const slot& s) { return s.active; }));
}

inline void materialize(chunk_batch& batch, const slot& completed, std::size_t count) {
#ifdef C07_PIPELINE_FAILURE_TEST
    if (inject_materialization_failure) {
        inject_materialization_failure = false;
        ++injected_failures;
        throw std::bad_alloc{}; // Same unwind boundary as the following owned-byte allocation.
    }
#endif
    completed_chunk chunk{completed.context.request_id, completed.offset,
        {completed.context.buffer.begin(), completed.context.buffer.begin() + count}};
    batch.chunks.push_back(std::move(chunk));
    batch.metrics.application_copy_bytes += count;
}

#ifdef _WIN32
class pending_guard {
    native_handle file_;
    native_handle port_;
    std::span<slot> slots_;
public:
    pending_guard(native_handle file, native_handle port, std::span<slot> slots) noexcept
        : file_(file), port_(port), slots_(slots) {}
    ~pending_guard() noexcept {
        if (!outstanding(slots_)) return;
        for (auto& entry : slots_) if (entry.active) (void)::CancelIoEx(file_, &entry.context.overlapped);
        const auto deadline = clock::now() + std::chrono::seconds(5);
        while (outstanding(slots_)) {
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - clock::now()).count();
            if (left <= 0) completion_probe_fatal_undrained("pipeline cancellation deadline", std::make_error_code(std::errc::timed_out));
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            OVERLAPPED* completed = nullptr;
            (void)::GetQueuedCompletionStatus(port_, &bytes, &key, &completed, static_cast<DWORD>(left));
            if (!completed) completion_probe_fatal_undrained("pipeline cancellation dequeue", last_error_code());
            auto found = std::find_if(slots_.begin(), slots_.end(), [completed](const slot& s) {
                return s.active && &s.context.overlapped == completed;
            });
            if (found == slots_.end()) completion_probe_fatal_undrained("pipeline cancellation identity", invalid());
            found->active = false; // Both successful and failed completion retire the buffer loan.
#ifdef C07_PIPELINE_FAILURE_TEST
            ++cleanup_retirements;
#endif
        }
    }
};

inline std::expected<void, std::error_code> completion_read(native_handle file, chunk_batch& batch,
    std::size_t chunk_size, std::size_t concurrency, clock::time_point started) {
    unique_handle port{::CreateIoCompletionPort(file, nullptr, 1, 1)};
    if (!port) return std::unexpected(last_error_code());
    std::vector<slot> slots(concurrency);
    for (auto& entry : slots) entry.context.buffer.resize(chunk_size);
    // Declared after slots: even allocation exceptions drain before buffers die.
    pending_guard pending{file, port.get(), slots};
    batch.metrics.setup_seconds = seconds(started);
    const auto reading = clock::now();
    std::size_t next = 0;
    auto submit = [&](slot& entry) -> std::expected<void, std::error_code> {
        entry.offset = next;
        entry.requested = std::min(chunk_size, batch.total_bytes - next);
        entry.context.request_id = next / chunk_size + 1;
        entry.context.overlapped = {};
        entry.context.overlapped.Offset = static_cast<DWORD>(next);
        entry.context.overlapped.OffsetHigh = static_cast<DWORD>(static_cast<std::uint64_t>(next) >> 32);
        ++batch.metrics.read_calls;
        const BOOL ok = ::ReadFile(file, entry.context.buffer.data(), static_cast<DWORD>(entry.requested),
            nullptr, &entry.context.overlapped);
        const DWORD error = ok ? ERROR_SUCCESS : ::GetLastError();
        if (!ok && error != ERROR_IO_PENDING) return std::unexpected(std::error_code{static_cast<int>(error), std::system_category()});
        entry.active = true; // Immediate TRUE still owes its default IOCP completion packet.
        next += entry.requested;
        batch.metrics.peak_in_flight = std::max(batch.metrics.peak_in_flight, outstanding(slots));
        return {};
    };
    for (auto& entry : slots) {
        if (next == batch.total_bytes) break;
        if (auto result = submit(entry); !result) return result;
    }
    while (outstanding(slots)) {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        OVERLAPPED* completed = nullptr;
        const BOOL ok = ::GetQueuedCompletionStatus(port.get(), &bytes, &key, &completed, 10000);
        const DWORD error = ok ? ERROR_SUCCESS : ::GetLastError();
        if (!completed) return std::unexpected(std::error_code{static_cast<int>(error), std::system_category()});
        auto found = std::find_if(slots.begin(), slots.end(), [completed](const slot& entry) {
            return entry.active && &entry.context.overlapped == completed;
        });
        if (found == slots.end() || key != 1) return std::unexpected(invalid());
        found->active = false;
        ++batch.metrics.completions;
        if (!ok) return std::unexpected(std::error_code{static_cast<int>(error), std::system_category()});
        if (bytes != found->requested) return std::unexpected(std::make_error_code(std::errc::io_error));
        materialize(batch, *found, bytes);
        if (next < batch.total_bytes) if (auto result = submit(*found); !result) return result;
    }
    batch.metrics.read_seconds = seconds(reading);
    return {};
}
#elif defined(C07_HAS_LIBURING)
class ring_owner {
public:
    io_uring ring{};
    bool initialized{};
    ~ring_owner() { if (initialized) io_uring_queue_exit(&ring); }
};

class pending_guard {
    io_uring& ring_;
    std::span<slot> slots_;
    static constexpr std::uint64_t cancel_bit = std::uint64_t{1} << 63;
public:
    pending_guard(io_uring& ring, std::span<slot> slots) noexcept : ring_(ring), slots_(slots) {}
    ~pending_guard() noexcept {
        if (!outstanding(slots_)) return;
        std::array<bool, 16> cancel_pending{};
        unsigned controls = 0;
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            auto& entry = slots_[i];
            if (!entry.active) continue;
            auto* sqe = io_uring_get_sqe(&ring_);
            if (!sqe) completion_probe_fatal_undrained("pipeline cancel SQ capacity", invalid());
            io_uring_prep_cancel64(sqe, entry.context.request_id, 0);
            io_uring_sqe_set_data64(sqe, entry.context.request_id | cancel_bit);
            cancel_pending[i] = true;
            ++controls;
        }
        const int submitted = io_uring_submit(&ring_);
        if (submitted != static_cast<int>(controls)) completion_probe_fatal_undrained("pipeline cancel submission", std::make_error_code(std::errc::io_error));
        const auto deadline = clock::now() + std::chrono::seconds(5);
        while (outstanding(slots_) || controls) {
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - clock::now()).count();
            if (ns <= 0) completion_probe_fatal_undrained("pipeline cancel deadline", std::make_error_code(std::errc::timed_out));
            __kernel_timespec timeout{ns / 1000000000, ns % 1000000000};
            io_uring_cqe* cqe = nullptr;
            const int rc = io_uring_wait_cqe_timeout(&ring_, &cqe, &timeout);
            if (rc < 0) completion_probe_fatal_undrained("pipeline cancel dequeue", std::error_code{-rc, std::generic_category()});
            const auto id = io_uring_cqe_get_data64(cqe);
            io_uring_cqe_seen(&ring_, cqe);
            auto found = std::find_if(slots_.begin(), slots_.end(), [id](const slot& entry) {
                return entry.context.request_id == (id & ~cancel_bit);
            });
            if (found == slots_.end()) completion_probe_fatal_undrained("pipeline cancel identity", invalid());
            if (id & cancel_bit) {
                const auto index = static_cast<std::size_t>(found - slots_.begin());
                if (!cancel_pending[index]) completion_probe_fatal_undrained("pipeline duplicate cancel", invalid());
                cancel_pending[index] = false;
                --controls;
            } else {
                if (!found->active) completion_probe_fatal_undrained("pipeline duplicate target", invalid());
                found->active = false;
#ifdef C07_PIPELINE_FAILURE_TEST
                ++cleanup_retirements;
#endif
            }
        }
    }
};

inline std::expected<void, std::error_code> completion_read(native_handle file, chunk_batch& batch,
    std::size_t chunk_size, std::size_t concurrency, clock::time_point started) {
    ring_owner owner;
    const int init = io_uring_queue_init(static_cast<unsigned>(2 * concurrency + 2), &owner.ring, 0);
    if (init < 0) return std::unexpected(is_known_uring_capability_error(-init)
        ? unsupported_completion_capability() : std::error_code{-init, std::generic_category()});
    owner.initialized = true;
    auto* probe = io_uring_get_probe_ring(&owner.ring);
    if (!probe) return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
    const bool supported = io_uring_opcode_supported(probe, IORING_OP_READ)
        && io_uring_opcode_supported(probe, IORING_OP_ASYNC_CANCEL);
    io_uring_free_probe(probe);
    if (!supported) return std::unexpected(unsupported_completion_capability());
    std::vector<slot> slots(concurrency);
    for (auto& entry : slots) entry.context.buffer.resize(chunk_size);
    pending_guard pending{owner.ring, slots};
    batch.metrics.setup_seconds = seconds(started);
    const auto reading = clock::now();
    std::size_t next = 0;
    auto submit = [&](slot& entry) {
        entry.offset = next;
        entry.requested = std::min(chunk_size, batch.total_bytes - next);
        entry.context.request_id = next / chunk_size + 1;
        auto* sqe = io_uring_get_sqe(&owner.ring);
        if (!sqe) completion_probe_fatal_undrained("pipeline read SQ capacity", invalid());
        io_uring_prep_read(sqe, file, entry.context.buffer.data(), static_cast<unsigned>(entry.requested), entry.offset);
        io_uring_sqe_set_data64(sqe, entry.context.request_id);
        ++batch.metrics.read_calls;
        const int submitted = io_uring_submit(&owner.ring);
        // This teaching engine submits one SQE, with SQPOLL disabled. An ambiguous
        // short/error submission is fatal, never unwinds possibly published buffers.
        if (submitted != 1) completion_probe_fatal_undrained("pipeline read submission", std::make_error_code(std::errc::io_error));
        entry.active = true;
        next += entry.requested;
        batch.metrics.peak_in_flight = std::max(batch.metrics.peak_in_flight, outstanding(slots));
    };
    for (auto& entry : slots) {
        if (next == batch.total_bytes) break;
        submit(entry);
    }
    while (outstanding(slots)) {
        io_uring_cqe* cqe = nullptr;
        __kernel_timespec timeout{10, 0};
        const int rc = io_uring_wait_cqe_timeout(&owner.ring, &cqe, &timeout);
        if (rc < 0) return std::unexpected(std::error_code{-rc, std::generic_category()});
        const auto id = io_uring_cqe_get_data64(cqe);
        const int count = cqe->res;
        io_uring_cqe_seen(&owner.ring, cqe);
        auto found = std::find_if(slots.begin(), slots.end(), [id](const slot& entry) {
            return entry.active && entry.context.request_id == id;
        });
        if (found == slots.end()) return std::unexpected(invalid());
        found->active = false;
        ++batch.metrics.completions;
        if (count < 0) return std::unexpected(std::error_code{-count, std::generic_category()});
        if (static_cast<std::size_t>(count) != found->requested) return std::unexpected(std::make_error_code(std::errc::io_error));
        materialize(batch, *found, static_cast<std::size_t>(count));
        if (next < batch.total_bytes) submit(*found);
    }
    batch.metrics.read_seconds = seconds(reading);
    return {};
}
#endif
} // namespace c07::pipeline_detail

namespace c07 {
inline std::expected<chunk_batch, std::error_code> read_file_chunks(const std::filesystem::path& path,
    file_backend mode, std::size_t chunk_size, std::size_t in_flight) {
    using namespace pipeline_detail;
    if (!chunk_size || chunk_size > 1024 * 1024 || !in_flight || in_flight > 16)
        return std::unexpected(invalid());
    if (mode != file_backend::buffered && mode != file_backend::mapped && mode != file_backend::completion)
        return std::unexpected(invalid());
#if !defined(_WIN32) && !defined(C07_HAS_LIBURING)
    if (mode == file_backend::completion) return std::unexpected(unsupported_completion_capability());
#endif
    try {
        const auto started = clock::now();
        auto file = open_source(path, mode);
        if (!file) return std::unexpected(file.error());
        auto size = source_size(file->get());
        if (!size) return std::unexpected(size.error());
        const auto chunks = *size / chunk_size + (*size % chunk_size != 0);
        if (chunks > max_pipeline_chunks) return std::unexpected(std::make_error_code(std::errc::value_too_large));
        chunk_batch batch;
        batch.total_bytes = *size;
        batch.chunks.reserve(chunks);
        if (!*size) { batch.metrics.setup_seconds = seconds(started); return batch; }
        if (mode == file_backend::mapped) {
            auto mapping = map_readonly(path, 0, *size);
            if (!mapping) return std::unexpected(mapping.error());
            if (mapping->bytes().size() != *size) return std::unexpected(std::make_error_code(std::errc::io_error));
            batch.metrics.setup_seconds = seconds(started);
            const auto reading = clock::now();
            for (std::size_t offset = 0; offset < *size;) {
                const auto length = std::min(chunk_size, *size - offset);
                const auto window = mapping->bytes().subspan(offset, length);
                batch.chunks.push_back({offset / chunk_size + 1, offset, {window.begin(), window.end()}});
                batch.metrics.application_copy_bytes += length;
                offset += length;
            }
            batch.metrics.read_seconds = seconds(reading);
        } else if (mode == file_backend::buffered) {
            batch.metrics.setup_seconds = seconds(started);
            const auto reading = clock::now();
            for (std::size_t offset = 0; offset < *size;) {
                const auto length = std::min(chunk_size, *size - offset);
                completed_chunk chunk{offset / chunk_size + 1, offset, std::vector<std::byte>(length)};
                for (std::size_t filled = 0; filled < length;) {
                    ++batch.metrics.read_calls;
                    auto count = read_some(file->get(), std::span(chunk.bytes).subspan(filled));
                    if (!count) return std::unexpected(count.error());
                    if (!*count) return std::unexpected(std::make_error_code(std::errc::io_error));
                    filled += *count;
                }
                batch.chunks.push_back(std::move(chunk));
                offset += length;
            }
            batch.metrics.read_seconds = seconds(reading);
        } else {
#if defined(_WIN32) || defined(C07_HAS_LIBURING)
            auto result = completion_read(file->get(), batch, chunk_size, std::min(in_flight, chunks), started);
            if (!result) return std::unexpected(result.error());
#endif
        }
        // ponytail: retain at most 64 MiB/65536 completed fragments; use a bounded
        // streaming ordered sink if the teaching contract grows to large files.
        return batch;
    } catch (const std::bad_alloc&) {
        return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
    }
}

inline std::expected<void, std::error_code> write_new_file(const std::filesystem::path& path,
    std::span<const std::byte> bytes) {
    if (bytes.size() > max_pipeline_bytes) return std::unexpected(std::make_error_code(std::errc::file_too_large));
    auto file = create_new_file(path);
    if (!file) return std::unexpected(file.error());
    if (auto written = write_all(file->get(), bytes); !written) return written;
#ifdef _WIN32
    if (!::FlushFileBuffers(file->get())) return std::unexpected(last_error_code());
    const auto native = file->release();
    if (!::CloseHandle(native)) return std::unexpected(last_error_code());
#else
    while (::fsync(file->get()) != 0) {
        if (errno != EINTR) return std::unexpected(last_error_code());
    }
    const auto native = file->release();
    if (::close(native) != 0) return std::unexpected(last_error_code()); // Linux: never retry close.
#endif
    return {};
}
} // namespace c07
