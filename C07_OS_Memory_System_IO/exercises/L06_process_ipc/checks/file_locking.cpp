#include <check.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>

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
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

std::filesystem::path temp_file() {
    const auto stamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("c07_l06_file_lock_" + stamp + ".bin");
}

#ifdef _WIN32
class unique_handle {
public:
    unique_handle() = default;
    explicit unique_handle(HANDLE handle) : handle_(handle) {}
    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;
    unique_handle(unique_handle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    unique_handle& operator=(unique_handle&& other) noexcept {
        if (this != &other) reset(std::exchange(other.handle_, nullptr));
        return *this;
    }
    ~unique_handle() { reset(); }
    HANDLE get() const { return handle_; }
    explicit operator bool() const { return handle_ && handle_ != INVALID_HANDLE_VALUE; }
    void reset(HANDLE next = nullptr) {
        if (*this) (void)::CloseHandle(handle_);
        handle_ = next;
    }
private:
    HANDLE handle_ = nullptr;
};

unique_handle open_shared(const std::filesystem::path& path) {
    return unique_handle{::CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
}

bool lock_range(HANDLE file, DWORD offset, DWORD length) {
    OVERLAPPED overlapped{};
    overlapped.Offset = offset;
    return ::LockFileEx(file, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
        0, length, 0, &overlapped) != 0;
}

bool unlock_range(HANDLE file, DWORD offset, DWORD length) {
    OVERLAPPED overlapped{};
    overlapped.Offset = offset;
    return ::UnlockFileEx(file, 0, length, 0, &overlapped) != 0;
}

void run_windows() {
    const auto path = temp_file();
    { std::ofstream(path, std::ios::binary) << "0123456789abcdef0123456789abcdef"; }
    auto first = open_shared(path);
    auto second = open_shared(path);
    check(first && second, "open two shared handles");

    check(lock_range(first.get(), 0, 8), "first handle locks byte range");
    check(!lock_range(second.get(), 4, 4), "overlapping byte lock is rejected");
    check(lock_range(second.get(), 16, 8), "non-overlapping byte lock is allowed");
    check(unlock_range(second.get(), 16, 8), "unlock non-overlap");
    check(unlock_range(first.get(), 0, 8), "unlock first range");
    check(lock_range(second.get(), 4, 4), "overlap succeeds after unlock");
    check(unlock_range(second.get(), 4, 4), "unlock retry range");

    first.reset();
    second.reset();
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::cout << "Windows LockFileEx: overlap refused, non-overlap allowed, retry after unlock allowed\n";
}
#else
struct child_reply {
    int open_errno = 0;
    int lock_errno = 0;
    int locked = 0;
};

bool set_lock(int fd, short type, off_t start, off_t len) {
    flock lock{};
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = start;
    lock.l_len = len;
    return ::fcntl(fd, F_SETLK, &lock) == 0;
}

child_reply child_try_lock(const char* path, off_t start, off_t len) {
    int pipefd[2] = {-1, -1};
    check(::pipe(pipefd) == 0, "create child result pipe");
    const pid_t pid = ::fork();
    check(pid >= 0, "fork lock contender");
    if (pid == 0) {
        (void)::close(pipefd[0]);
        child_reply reply{};
        const int fd = ::open(path, O_RDWR | O_CLOEXEC);
        if (fd == -1) {
            reply.open_errno = errno;
        } else {
            flock lock{};
            lock.l_type = F_WRLCK;
            lock.l_whence = SEEK_SET;
            lock.l_start = start;
            lock.l_len = len;
            if (::fcntl(fd, F_SETLK, &lock) == 0) reply.locked = 1;
            else reply.lock_errno = errno;
            (void)::close(fd);
        }
        const unsigned char* next = reinterpret_cast<const unsigned char*>(&reply);
        std::size_t left = sizeof(reply);
        while (left != 0) {
            const ssize_t n = ::write(pipefd[1], next, left);
            if (n > 0) {
                next += n;
                left -= static_cast<std::size_t>(n);
            } else if (n == -1 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        ::_exit(reply.open_errno == 0 ? 0 : 2);
    }
    (void)::close(pipefd[1]);

    child_reply reply{};
    unsigned char* next = reinterpret_cast<unsigned char*>(&reply);
    std::size_t left = sizeof(reply);
    while (left != 0) {
        const ssize_t n = ::read(pipefd[0], next, left);
        if (n > 0) {
            next += n;
            left -= static_cast<std::size_t>(n);
        } else if (n == -1 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    (void)::close(pipefd[0]);

    int status = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    for (;;) {
        const pid_t done = ::waitpid(pid, &status, WNOHANG);
        if (done == pid) break;
        if (done == -1 && errno == EINTR) continue;
        if (std::chrono::steady_clock::now() >= deadline) {
            (void)::kill(pid, SIGKILL);
            (void)::waitpid(pid, &status, 0);
            check(false, "child lock contender timed out");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    check(WIFEXITED(status), "child lock contender reaped");
    return reply;
}

void run_linux() {
    const auto path = temp_file();
    const std::string initial = "0123456789abcdef0123456789abcdef";
    { std::ofstream(path, std::ios::binary) << initial; }
    const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
    check(fd >= 0, "open lock file");

    check(set_lock(fd, F_WRLCK, 0, 8), "parent locks byte range");
    const auto overlap = child_try_lock(path.c_str(), 4, 4);
    check(!overlap.locked && (overlap.lock_errno == EACCES || overlap.lock_errno == EAGAIN),
        "overlapping child F_SETLK is rejected");
    const auto separate = child_try_lock(path.c_str(), 16, 8);
    check(separate.locked, "non-overlapping child F_SETLK is allowed");
    check(set_lock(fd, F_UNLCK, 0, 8), "parent unlocks byte range");
    const auto retry = child_try_lock(path.c_str(), 4, 4);
    check(retry.locked, "overlap succeeds after unlock in another process");

    check(::lseek(fd, 2, SEEK_SET) == 2, "set shared file offset");
    const char mark = 'X';
    check(::pwrite(fd, &mark, 1, 12) == 1, "pwrite at explicit offset");
    check(::lseek(fd, 0, SEEK_CUR) == 2, "pwrite does not advance shared file offset");
    char got = 0;
    check(::pread(fd, &got, 1, 12) == 1 && got == 'X', "pread reads explicit offset");
    check(::lseek(fd, 0, SEEK_CUR) == 2, "pread does not advance shared file offset");

    (void)::close(fd);
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::cout << "Linux fcntl: child overlap refused, non-overlap allowed, retry after unlock allowed; pread/pwrite kept file offset\n";
}
#endif

} // namespace

int main() {
#ifdef _WIN32
    run_windows();
#else
    run_linux();
#endif
    std::cout << "L06 file locking observation passed\n";
}
