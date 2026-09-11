#pragma once

#include <c07/os.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <new>
#include <span>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(C07_HAS_LIBURING)
#include <liburing.h>
#endif
#endif

namespace c10_native {

inline constexpr std::size_t max_read_bytes = 1024 * 1024;
inline constexpr std::size_t max_record_lines = 4096;

struct read_result {
  std::size_t bytes{};
};

enum class capability_state { ready, unsupported, failed };

struct completion {
  std::size_t bytes{};
  std::error_code error;
  bool stopped{};
};

inline auto unsupported_native_io() -> std::error_code {
  return std::make_error_code(std::errc::function_not_supported);
}

#ifndef _WIN32
inline bool is_capability_error(int err) {
  return err == ENOSYS || err == EOPNOTSUPP || err == EPERM;
}
#endif

#ifndef _WIN32
[[noreturn]] inline void fatal_undrained(const char *stage, std::error_code error) noexcept {
  std::fprintf(stderr, "c10 native_io fatal undrained at %s: %s\n", stage, error.message().c_str());
  std::_Exit(70);
}
#endif

class io_context;

struct native_file {
  c07::unique_file handle;
  std::uint64_t size{};
  io_context *context{};
};

class read_request {
public:
  using callback_t = std::function<void(completion)>;

  read_request(std::shared_ptr<native_file> file, std::uint64_t offset, std::span<std::byte> buffer,
               callback_t done)
      : file_(std::move(file)), offset_(offset), buffer_(buffer), done_(std::move(done)) {}
  read_request(const read_request &) = delete;
  auto operator=(const read_request &) -> read_request & = delete;

  void cancel() noexcept;

private:
  friend class io_context;

  void finish(completion event) noexcept {
    callback_t done;
    {
      std::lock_guard lock{mutex_};
      if (completed_.exchange(true, std::memory_order_acq_rel))
        return;
      done = std::move(done_);
    }
    done(event);
  }

  std::shared_ptr<native_file> file_;
  std::uint64_t offset_{};
  std::span<std::byte> buffer_;
  callback_t done_;
  std::mutex mutex_;
  std::atomic_bool completed_{false};
#ifdef _WIN32
  struct win_packet {
    OVERLAPPED overlapped{};
    read_request *owner{};
  } packet_;
#endif
};

class io_context {
public:
  // ponytail: owner serializes open/submit/close; add an admission lock if close must race
  // submissions. Completion and stop callbacks may run concurrently; keep this context alive until
  // they retire.
  io_context() { open(); }
  io_context(const io_context &) = delete;
  auto operator=(const io_context &) -> io_context & = delete;
  ~io_context() { close(); }

  [[nodiscard]] auto state() const noexcept -> capability_state { return state_; }
  [[nodiscard]] auto available() const noexcept -> bool {
    return state_ == capability_state::ready;
  }
  [[nodiscard]] auto last_error() const noexcept -> std::error_code { return error_; }

  auto open_file(const std::filesystem::path &path)
      -> std::expected<std::shared_ptr<native_file>, std::error_code> {
    if (!available())
      return std::unexpected(error_ ? error_ : unsupported_native_io());
    if (closing_.load(std::memory_order_acquire)) {
      return std::unexpected(std::make_error_code(std::errc::operation_canceled));
    }
#ifdef _WIN32
    c07::unique_file file{::CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ,
                                        nullptr, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr)};
    if (!file)
      return std::unexpected(c07::last_error_code());
    BY_HANDLE_FILE_INFORMATION info{};
    LARGE_INTEGER size{};
    if (!::GetFileInformationByHandle(file.get(), &info) || !::GetFileSizeEx(file.get(), &size)) {
      return std::unexpected(c07::last_error_code());
    }
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || size.QuadPart < 0) {
      return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }
    if (static_cast<std::uint64_t>(size.QuadPart) > max_read_bytes) {
      return std::unexpected(std::make_error_code(std::errc::file_too_large));
    }
    if (::CreateIoCompletionPort(file.get(), port_.get(), 0, 1) == nullptr) {
      return std::unexpected(c07::last_error_code());
    }
    return std::make_shared<native_file>(
        native_file{std::move(file), static_cast<std::uint64_t>(size.QuadPart), this});
#else
    int fd = -1;
    do {
      fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0)
      return std::unexpected(c07::last_error_code());
    c07::unique_file file{fd};
    struct stat info{};
    if (::fstat(file.get(), &info) != 0)
      return std::unexpected(c07::last_error_code());
    if (!S_ISREG(info.st_mode) || info.st_size < 0) {
      return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }
    if (static_cast<std::uint64_t>(info.st_size) > max_read_bytes) {
      return std::unexpected(std::make_error_code(std::errc::file_too_large));
    }
    int flags = ::fcntl(file.get(), F_GETFL, 0);
    if (flags >= 0)
      (void)::fcntl(file.get(), F_SETFL, flags & ~O_NONBLOCK);
    return std::make_shared<native_file>(
        native_file{std::move(file), static_cast<std::uint64_t>(info.st_size), this});
#endif
  }

  auto async_read_at(std::shared_ptr<native_file> file, std::uint64_t offset,
                     std::span<std::byte> buffer, read_request::callback_t done)
      -> std::expected<std::shared_ptr<read_request>, std::error_code> {
    if (!available())
      return std::unexpected(error_ ? error_ : unsupported_native_io());
    if (!file || file->context != this || buffer.size() > max_read_bytes || offset > file->size) {
      return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }
    auto request = std::make_shared<read_request>(std::move(file), offset, buffer, std::move(done));
    if (buffer.empty()) {
      request->finish({});
      return request;
    }
    auto error = submit(request);
    if (error)
      return std::unexpected(error);
    return request;
  }

  void close() noexcept {
    bool expected = false;
    if (!closing_.compare_exchange_strong(expected, true))
      return;
#ifdef _WIN32
    if (port_)
      (void)::PostQueuedCompletionStatus(port_.get(), 0, 0, nullptr);
#endif
    if (worker_.joinable())
      worker_.join();
#if !defined(_WIN32) && defined(C07_HAS_LIBURING)
    if (ring_ready_)
      io_uring_queue_exit(&ring_);
#endif
  }

  void cancel(read_request *request) noexcept {
#ifdef _WIN32
    (void)::CancelIoEx(request->file_->handle.get(), &request->packet_.overlapped);
#elif defined(C07_HAS_LIBURING)
    if (!available())
      return;
    std::lock_guard lock{ring_mutex_};
    auto *sqe = io_uring_get_sqe(&ring_);
    if (!sqe)
      return;
    io_uring_prep_cancel64(sqe, reinterpret_cast<std::uintptr_t>(request), 0);
    io_uring_sqe_set_data64(sqe, 0);
    control_in_flight_.fetch_add(1, std::memory_order_acq_rel);
    const int submitted = io_uring_submit(&ring_);
    if (submitted != 1) {
      const auto ec = submitted < 0 ? std::error_code{-submitted, std::generic_category()}
                                    : std::make_error_code(std::errc::io_error);
      fatal_undrained("io_uring cancel submit", ec);
    }
#else
    (void)request;
#endif
  }

private:
  void open() {
#ifdef _WIN32
    port_.reset(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
    if (!port_) {
      state_ = capability_state::failed;
      error_ = c07::last_error_code();
      return;
    }
    try {
      worker_ = std::thread([this] { run(); });
      state_ = capability_state::ready;
    } catch (const std::system_error &err) {
      error_ = err.code();
      state_ = capability_state::failed;
      port_.reset();
    }
#elif defined(C07_HAS_LIBURING)
    const int rc = io_uring_queue_init(64, &ring_, 0);
    if (rc < 0) {
      const int err = -rc;
      error_ = std::error_code{err, std::generic_category()};
      state_ = is_capability_error(err) ? capability_state::unsupported : capability_state::failed;
      return;
    }
    ring_ready_ = true;
    try {
      worker_ = std::thread([this] { run(); });
      state_ = capability_state::ready;
    } catch (const std::system_error &err) {
      error_ = err.code();
      state_ = capability_state::failed;
      io_uring_queue_exit(&ring_);
      ring_ready_ = false;
    }
#else
    state_ = capability_state::unsupported;
    error_ = unsupported_native_io();
#endif
  }

  auto submit(const std::shared_ptr<read_request> &request) noexcept -> std::error_code {
    if (closing_.load(std::memory_order_acquire))
      return std::make_error_code(std::errc::operation_canceled);
    in_flight_.fetch_add(1, std::memory_order_acq_rel);
    // A completion can arrive before the submission call returns. Publish ownership first.
    try {
      keep_alive(request);
    } catch (const std::bad_alloc &) {
      in_flight_.fetch_sub(1, std::memory_order_acq_rel);
      return std::make_error_code(std::errc::not_enough_memory);
    }
#ifdef _WIN32
    request->packet_ = {};
    request->packet_.owner = request.get();
    request->packet_.overlapped.Offset = static_cast<DWORD>(request->offset_);
    request->packet_.overlapped.OffsetHigh = static_cast<DWORD>(request->offset_ >> 32);
    const DWORD n = static_cast<DWORD>(request->buffer_.size());
    const BOOL ok = ::ReadFile(request->file_->handle.get(), request->buffer_.data(), n, nullptr,
                               &request->packet_.overlapped);
    const DWORD err = ok ? ERROR_SUCCESS : ::GetLastError();
#ifdef C10_NATIVE_IO_TEST_POST_SUBMIT
    ::Sleep(10); // Checker-only: widen the early-completion window, after preserving GetLastError.
#endif
    if (!ok && err != ERROR_IO_PENDING) {
      forget_alive(request.get());
      in_flight_.fetch_sub(1, std::memory_order_acq_rel);
      return {static_cast<int>(err), std::system_category()};
    }
    return {};
#elif defined(C07_HAS_LIBURING)
    std::lock_guard lock{ring_mutex_};
    auto *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      forget_alive(request.get());
      in_flight_.fetch_sub(1, std::memory_order_acq_rel);
      return std::make_error_code(std::errc::resource_unavailable_try_again);
    }
    io_uring_prep_read(sqe, request->file_->handle.get(), request->buffer_.data(),
                       static_cast<unsigned>(request->buffer_.size()),
                       static_cast<off_t>(request->offset_));
    io_uring_sqe_set_data64(sqe, reinterpret_cast<std::uintptr_t>(request.get()));
    const int submitted = io_uring_submit(&ring_);
    if (submitted == 1)
      return {};
    const auto ec = submitted < 0 ? std::error_code{-submitted, std::generic_category()}
                                  : std::make_error_code(std::errc::io_error);
    fatal_undrained("io_uring read submit", ec);
#else
    forget_alive(request.get());
    in_flight_.fetch_sub(1, std::memory_order_acq_rel);
    return unsupported_native_io();
#endif
  }

  void keep_alive(std::shared_ptr<read_request> request) {
    std::lock_guard lock{alive_mutex_};
    alive_.push_back(std::move(request));
  }

  auto take_alive(read_request *request) -> std::shared_ptr<read_request> {
    std::lock_guard lock{alive_mutex_};
    auto it = std::find_if(alive_.begin(), alive_.end(),
                           [&](const auto &item) { return item.get() == request; });
    if (it == alive_.end())
      return {};
    auto out = std::move(*it);
    alive_.erase(it);
    return out;
  }

  void forget_alive(read_request *request) { (void)take_alive(request); }

  void complete(read_request *raw, completion event) noexcept {
    auto request = take_alive(raw);
    in_flight_.fetch_sub(1, std::memory_order_acq_rel);
    if (request)
      request->finish(event);
  }

  void run() noexcept {
#ifdef _WIN32
    while (true) {
      DWORD bytes = 0;
      ULONG_PTR key = 0;
      OVERLAPPED *overlapped = nullptr;
      const BOOL ok = ::GetQueuedCompletionStatus(port_.get(), &bytes, &key, &overlapped, 100);
      if (!overlapped) {
        if (closing_.load(std::memory_order_acquire) &&
            in_flight_.load(std::memory_order_acquire) == 0)
          break;
        continue;
      }
      auto *request =
          static_cast<read_request::win_packet *>(static_cast<void *>(overlapped))->owner;
      const DWORD native_error = ok ? ERROR_SUCCESS : ::GetLastError();
      if (native_error == ERROR_OPERATION_ABORTED) {
        complete(request, completion{0, {}, true});
      } else if (native_error == ERROR_HANDLE_EOF) {
        complete(request, completion{0, {}, false});
      } else {
        complete(request, completion{bytes,
                                     native_error == ERROR_SUCCESS
                                         ? std::error_code{}
                                         : std::error_code{static_cast<int>(native_error),
                                                           std::system_category()},
                                     false});
      }
    }
#elif defined(C07_HAS_LIBURING)
    while (true) {
      if (closing_.load(std::memory_order_acquire) &&
          in_flight_.load(std::memory_order_acquire) == 0 &&
          control_in_flight_.load(std::memory_order_acquire) == 0)
        break;
      io_uring_cqe *cqe = nullptr;
      __kernel_timespec timeout{0, 100000000};
      const int rc = io_uring_wait_cqe_timeout(&ring_, &cqe, &timeout);
      if (rc == -ETIME)
        continue;
      if (rc < 0) {
        error_ = std::error_code{-rc, std::generic_category()};
        state_ = capability_state::failed;
        fatal_undrained("io_uring wait", error_);
      }
      auto *request = reinterpret_cast<read_request *>(io_uring_cqe_get_data64(cqe));
      const int res = cqe->res;
      io_uring_cqe_seen(&ring_, cqe);
      if (request == nullptr) {
        control_in_flight_.fetch_sub(1, std::memory_order_acq_rel);
        continue;
      }
      complete(request, completion{res > 0 ? static_cast<std::size_t>(res) : 0,
                                   res < 0 && res != -ECANCELED
                                       ? std::error_code{-res, std::generic_category()}
                                       : std::error_code{},
                                   res == -ECANCELED});
    }
#endif
  }

  capability_state state_{capability_state::failed};
  std::error_code error_;
  std::thread worker_;
  std::atomic_bool closing_{false};
  std::atomic_size_t in_flight_{0};
  std::atomic_size_t control_in_flight_{0};
  std::mutex alive_mutex_;
  std::vector<std::shared_ptr<read_request>> alive_;
#ifdef _WIN32
  c07::unique_handle port_;
#elif defined(C07_HAS_LIBURING)
  io_uring ring_{};
  bool ring_ready_{false};
  std::mutex ring_mutex_;
#endif
};

inline void read_request::cancel() noexcept { file_->context->cancel(this); }

} // namespace c10_native
