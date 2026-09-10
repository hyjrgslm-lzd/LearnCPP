#ifndef C07_OS_MEMORY_SYSTEM_IO_COMPLETION_IO_HPP
#define C07_OS_MEMORY_SYSTEM_IO_COMPLETION_IO_HPP

#include <c07/os.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <ctime>
#include <unistd.h>
#if defined(C07_HAS_LIBURING)
#include <liburing.h>
#endif
#endif

namespace c07 {

enum class completion_event_kind {
    read_accepted,
    read_completed,
    cancel_submitted,
    cancel_completed,
    target_completed
};

struct completion_event {
    std::uint64_t request_id = 0;
    completion_event_kind kind = completion_event_kind::read_accepted;
    std::uint64_t target_request_id = 0;
    std::size_t bytes = 0;
    std::error_code error;
};

struct completion_probe_report {
    std::string payload;
    std::vector<completion_event> events;
};

inline completion_event read_accepted(std::uint64_t request_id, std::size_t expected_bytes) {
    return {request_id, completion_event_kind::read_accepted, 0, expected_bytes, {}};
}

inline completion_event read_completed(std::uint64_t request_id, std::size_t bytes,
    std::error_code error = {}) {
    return {request_id, completion_event_kind::read_completed, 0, bytes, error};
}

inline completion_event cancel_submitted(std::uint64_t cancel_request_id,
    std::uint64_t target_request_id, std::error_code error = {}) {
    return {cancel_request_id, completion_event_kind::cancel_submitted, target_request_id, 0, error};
}

inline completion_event cancel_completed(std::uint64_t cancel_request_id,
    std::uint64_t target_request_id, std::error_code error = {}) {
    return {cancel_request_id, completion_event_kind::cancel_completed, target_request_id, 0, error};
}

inline completion_event target_completed(std::uint64_t target_request_id, std::size_t bytes,
    std::error_code error = {}) {
    return {target_request_id, completion_event_kind::target_completed, 0, bytes, error};
}

inline std::unexpected<std::error_code> completion_probe_failure(
    std::string_view stage, std::error_code error) {
    std::cerr << "completion probe stage failed: " << stage << ": " << error.message() << '\n';
    return std::unexpected(error);
}

inline std::error_code unsupported_completion_capability() {
    return std::make_error_code(std::errc::function_not_supported);
}

#ifndef _WIN32
inline bool is_known_uring_capability_error(int err) {
    return err == ENOSYS || err == EPERM || err == EOPNOTSUPP;
}
#endif

[[noreturn]] inline void completion_probe_fatal_undrained(const char* stage, std::error_code error) noexcept {
    (void)std::fprintf(stderr, "completion probe fatal undrained request: %s: %s:%d\n",
        stage, error.category().name(), error.value());
    std::_Exit(70);
}

struct operation_context {
    std::uint64_t request_id = 0;
    std::vector<std::byte> buffer;
#ifdef _WIN32
    OVERLAPPED overlapped{};
#endif
};

#ifdef _WIN32
inline std::expected<completion_event, std::error_code> wait_iocp_event(
    native_handle port, operation_context& context, completion_event_kind kind, DWORD timeout_ms) {
    DWORD transferred = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED completed = nullptr;
    const BOOL ok = ::GetQueuedCompletionStatus(port, &transferred, &key, &completed, timeout_ms);
    const DWORD native_error = ok ? ERROR_SUCCESS : ::GetLastError();
    if (completed == nullptr) {
        return std::unexpected(std::error_code{static_cast<int>(native_error), std::system_category()});
    }
    if (completed != &context.overlapped) {
        return std::unexpected(std::make_error_code(std::errc::protocol_error));
    }
    const std::error_code error = ok ? std::error_code{}
                                     : std::error_code{static_cast<int>(native_error), std::system_category()};
    if (kind == completion_event_kind::read_completed) {
        return read_completed(context.request_id, transferred, error);
    }
    return target_completed(context.request_id, transferred, error);
}

inline void cancel_and_drain_iocp_or_exit(native_handle file, native_handle port, operation_context& context,
    completion_event_kind kind, const char* stage) {
    (void)::CancelIoEx(file, &context.overlapped);
    auto drained = wait_iocp_event(port, context, kind, 5000);
    if (!drained) completion_probe_fatal_undrained(stage, drained.error());
}

inline std::expected<completion_probe_report, std::error_code> run_completion_probe(std::string_view payload) {
    if (payload.empty()) payload = "empty payload still uses one byte";
    if (payload.size() > 4096) return std::unexpected(std::make_error_code(std::errc::message_size));

    completion_probe_report report;
    report.payload = std::string{payload};
    report.events.reserve(6);

    wchar_t temp_dir[MAX_PATH]{};
    if (::GetTempPathW(MAX_PATH, temp_dir) == 0) return completion_probe_failure("GetTempPathW", last_error_code());
    wchar_t temp_file[MAX_PATH]{};
    if (::GetTempFileNameW(temp_dir, L"c07", 0, temp_file) == 0) return completion_probe_failure("GetTempFileNameW", last_error_code());
    struct file_cleanup {
        const wchar_t* path;
        ~file_cleanup() { (void)::DeleteFileW(path); }
    } cleanup{temp_file};

    {
        unique_handle writer{::CreateFileW(temp_file, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_TEMPORARY, nullptr)};
        if (!writer) return completion_probe_failure("CreateFileW writer", last_error_code());
        if (auto write = write_all(writer.get(), bytes_of(payload)); !write) {
            return completion_probe_failure("write payload", write.error());
        }
    }

    unique_handle file{::CreateFileW(temp_file, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_OVERLAPPED, nullptr)};
    if (!file) return completion_probe_failure("CreateFileW overlapped file", last_error_code());
    unique_handle port{::CreateIoCompletionPort(file.get(), nullptr, 1, 1)};
    if (!port) return completion_probe_failure("CreateIoCompletionPort file", last_error_code());

    operation_context read_context{1001, std::vector<std::byte>(payload.size())};
    if (!::ReadFile(file.get(), read_context.buffer.data(), static_cast<DWORD>(payload.size()), nullptr,
            &read_context.overlapped)
        && ::GetLastError() != ERROR_IO_PENDING) {
        return completion_probe_failure("ReadFile payload", last_error_code());
    }
    report.events.push_back(read_accepted(read_context.request_id, payload.size()));
    auto read_event = wait_iocp_event(port.get(), read_context, completion_event_kind::read_completed, 5000);
    if (!read_event) {
        cancel_and_drain_iocp_or_exit(file.get(), port.get(), read_context,
            completion_event_kind::read_completed, "GQCS payload");
        return completion_probe_failure("GQCS payload", read_event.error());
    }
    report.events.push_back(*read_event);
    if (read_event->error) return completion_probe_failure("ReadFile completion", read_event->error);
    report.payload.assign(reinterpret_cast<const char*>(read_context.buffer.data()),
        reinterpret_cast<const char*>(read_context.buffer.data()) + read_event->bytes);

    const std::wstring watch_dir = std::wstring(temp_dir) + L"c07_cancel_" + std::to_wstring(::GetCurrentProcessId())
        + L"_" + std::to_wstring(::GetTickCount64());
    if (!::CreateDirectoryW(watch_dir.c_str(), nullptr)) {
        return completion_probe_failure("CreateDirectoryW watch", last_error_code());
    }
    struct dir_cleanup {
        const wchar_t* path;
        ~dir_cleanup() { (void)::RemoveDirectoryW(path); }
    } cleanup_dir{watch_dir.c_str()};
    unique_handle watch{::CreateFileW(watch_dir.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr)};
    if (!watch) return completion_probe_failure("CreateFileW watch dir", last_error_code());
    unique_handle watch_port{::CreateIoCompletionPort(watch.get(), nullptr, 2, 1)};
    if (!watch_port) return completion_probe_failure("CreateIoCompletionPort watch", last_error_code());

    operation_context pending_context{2001, std::vector<std::byte>(1024)};
    if (!::ReadDirectoryChangesW(watch.get(), pending_context.buffer.data(),
            static_cast<DWORD>(pending_context.buffer.size()), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME, nullptr,
            &pending_context.overlapped, nullptr)
        && ::GetLastError() != ERROR_IO_PENDING) {
        return completion_probe_failure("ReadDirectoryChangesW pending", last_error_code());
    }
    report.events.push_back(read_accepted(pending_context.request_id, pending_context.buffer.size()));
    const std::uint64_t cancel_id = 3001;
    report.events.push_back(cancel_submitted(cancel_id, pending_context.request_id));
    const BOOL cancel_ok = ::CancelIoEx(watch.get(), &pending_context.overlapped);
    const std::error_code cancel_error = cancel_ok ? std::error_code{}
        : std::error_code{static_cast<int>(::GetLastError()), std::system_category()};
    auto target_event = wait_iocp_event(watch_port.get(), pending_context,
        completion_event_kind::target_completed, 5000);
    if (!target_event) {
        cancel_and_drain_iocp_or_exit(watch.get(), watch_port.get(), pending_context,
            completion_event_kind::target_completed, "GQCS pending target");
        return completion_probe_failure("GQCS pending target", target_event.error());
    }
    report.events.push_back(*target_event);
    report.events.push_back(cancel_completed(cancel_id, pending_context.request_id, cancel_error));
    return report;
}
#else
inline std::expected<completion_probe_report, std::error_code> run_completion_probe(std::string_view payload) {
#if defined(C07_HAS_LIBURING)
    if (payload.empty()) payload = "empty payload still uses one byte";
    if (payload.size() > 4096) return std::unexpected(std::make_error_code(std::errc::message_size));

    io_uring ring{};
    if (int rc = io_uring_queue_init(8, &ring, 0); rc < 0) {
        const int err = -rc;
        if (is_known_uring_capability_error(err)) return std::unexpected(unsupported_completion_capability());
        return std::unexpected(std::error_code{err, std::generic_category()});
    }
    struct ring_guard {
        io_uring* ring;
        ~ring_guard() { io_uring_queue_exit(ring); }
    } cleanup{&ring};

    io_uring_probe* probe = io_uring_get_probe_ring(&ring);
    if (probe == nullptr) return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
    const bool read_supported = io_uring_opcode_supported(probe, IORING_OP_READ) != 0;
    const bool cancel_supported = io_uring_opcode_supported(probe, IORING_OP_ASYNC_CANCEL) != 0;
    io_uring_free_probe(probe);
    if (!read_supported || !cancel_supported) return std::unexpected(unsupported_completion_capability());

    completion_probe_report report;
    report.payload = std::string{payload};
    report.events.reserve(6);

    std::error_code temp_error;
    const auto temp_dir = std::filesystem::temp_directory_path(temp_error);
    if (temp_error) return std::unexpected(temp_error);
    std::string path = (temp_dir / "c07_completion_XXXXXX").string();
    int raw_fd = ::mkstemp(path.data());
    if (raw_fd < 0) return std::unexpected(last_error_code());
    unique_fd file{raw_fd};
    (void)::unlink(path.c_str());
    if (auto write = write_all(file.get(), bytes_of(payload)); !write) return std::unexpected(write.error());
    if (::lseek(file.get(), 0, SEEK_SET) < 0) return std::unexpected(last_error_code());

    operation_context file_read{1001, std::vector<std::byte>(payload.size())};
    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) return std::unexpected(std::make_error_code(std::errc::resource_unavailable_try_again));
    io_uring_prep_read(sqe, file.get(), file_read.buffer.data(), file_read.buffer.size(), 0);
    io_uring_sqe_set_data64(sqe, file_read.request_id);
    int submitted = io_uring_submit(&ring);
    if (submitted != 1) {
        if (submitted < 0) return std::unexpected(std::error_code{-submitted, std::generic_category()});
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }
    report.events.push_back(read_accepted(file_read.request_id, payload.size()));

    io_uring_cqe* cqe = nullptr;
    __kernel_timespec timeout{5, 0};
    if (int rc = io_uring_wait_cqe_timeout(&ring, &cqe, &timeout); rc < 0) {
        completion_probe_fatal_undrained("io_uring payload read", std::error_code{-rc, std::generic_category()});
    }
    const auto read_id = io_uring_cqe_get_data64(cqe);
    const int read_res = cqe->res;
    io_uring_cqe_seen(&ring, cqe);
    if (read_id != file_read.request_id) {
        completion_probe_fatal_undrained("io_uring payload identity",
            std::make_error_code(std::errc::protocol_error));
    }
    if (read_res < 0) return std::unexpected(std::error_code{-read_res, std::generic_category()});
    report.events.push_back(read_completed(file_read.request_id, static_cast<std::size_t>(read_res)));
    report.payload.assign(reinterpret_cast<const char*>(file_read.buffer.data()),
        reinterpret_cast<const char*>(file_read.buffer.data()) + read_res);

    int fds[2]{-1, -1};
    if (::pipe(fds) != 0) return std::unexpected(last_error_code());
    unique_fd in{fds[0]};
    unique_fd out{fds[1]};
    operation_context pending_read{2001, std::vector<std::byte>(1)};
    sqe = io_uring_get_sqe(&ring);
    if (!sqe) return std::unexpected(std::make_error_code(std::errc::resource_unavailable_try_again));
    io_uring_prep_read(sqe, in.get(), pending_read.buffer.data(), pending_read.buffer.size(), -1);
    io_uring_sqe_set_data64(sqe, pending_read.request_id);
    submitted = io_uring_submit(&ring);
    if (submitted != 1) {
        if (submitted < 0) return std::unexpected(std::error_code{-submitted, std::generic_category()});
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }
    report.events.push_back(read_accepted(pending_read.request_id, pending_read.buffer.size()));

    const std::uint64_t cancel_id = 3001;
    sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        out.reset();
        if (io_uring_wait_cqe_timeout(&ring, &cqe, &timeout) == 0) {
            io_uring_cqe_seen(&ring, cqe);
        } else {
            completion_probe_fatal_undrained("io_uring cancel SQE unavailable",
                std::make_error_code(std::errc::resource_unavailable_try_again));
        }
        return std::unexpected(std::make_error_code(std::errc::resource_unavailable_try_again));
    }
    io_uring_prep_cancel64(sqe, pending_read.request_id, 0);
    io_uring_sqe_set_data64(sqe, cancel_id);
    submitted = io_uring_submit(&ring);
    if (submitted != 1) {
        const std::error_code submit_error = submitted < 0
            ? std::error_code{-submitted, std::generic_category()}
            : std::make_error_code(std::errc::io_error);
        out.reset();
        if (io_uring_wait_cqe_timeout(&ring, &cqe, &timeout) == 0) {
            io_uring_cqe_seen(&ring, cqe);
        } else {
            completion_probe_fatal_undrained("io_uring cancel submit", submit_error);
        }
        return std::unexpected(submit_error);
    }
    report.events.push_back(cancel_submitted(cancel_id, pending_read.request_id));

    bool saw_target = false;
    bool saw_cancel = false;
    for (int seen = 0; seen != 2; ++seen) {
        if (int rc = io_uring_wait_cqe_timeout(&ring, &cqe, &timeout); rc < 0) {
            completion_probe_fatal_undrained("io_uring cancel drain", std::error_code{-rc, std::generic_category()});
        }
        const auto id = io_uring_cqe_get_data64(cqe);
        const int res = cqe->res;
        io_uring_cqe_seen(&ring, cqe);
        const std::error_code event_error = res < 0 ? std::error_code{-res, std::generic_category()}
                                                    : std::error_code{};
        if (id == pending_read.request_id && !saw_target) {
            saw_target = true;
            report.events.push_back(target_completed(pending_read.request_id,
                res > 0 ? static_cast<std::size_t>(res) : 0, event_error));
        } else if (id == cancel_id && !saw_cancel) {
            saw_cancel = true;
            report.events.push_back(cancel_completed(cancel_id, pending_read.request_id, event_error));
        } else {
            completion_probe_fatal_undrained("io_uring unknown completion",
                std::make_error_code(std::errc::protocol_error));
        }
    }
    if (!saw_target || !saw_cancel) return std::unexpected(std::make_error_code(std::errc::protocol_error));
    return report;
#else
    (void)payload;
    return std::unexpected(unsupported_completion_capability());
#endif
}
#endif

} // namespace c07

#endif
