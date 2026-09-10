#pragma once
#include <include/process_ipc_contract.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

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
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace c07_l06 {
namespace good_detail {

#ifdef _WIN32
class win_handle {
public:
    win_handle() = default;
    explicit win_handle(HANDLE value) : value_(value) {}
    win_handle(const win_handle&) = delete;
    win_handle& operator=(const win_handle&) = delete;
    win_handle(win_handle&& other) noexcept : value_(std::exchange(other.value_, nullptr)) {}
    win_handle& operator=(win_handle&& other) noexcept {
        if (this != &other) close(std::exchange(other.value_, nullptr));
        return *this;
    }
    ~win_handle() { close(); }

    HANDLE get() const { return value_; }
    HANDLE* receive() { close(); return &value_; }
    explicit operator bool() const { return value_ != nullptr && value_ != INVALID_HANDLE_VALUE; }
    void close(HANDLE next = nullptr) {
        if (*this) (void)::CloseHandle(value_);
        value_ = next;
    }

private:
    HANDLE value_ = nullptr;
};

inline std::string last_error(std::string where) {
    where += ": ";
    where += std::to_string(::GetLastError());
    return where;
}

inline std::wstring quoted(std::wstring_view text) {
    std::wstring out;
    out.reserve(text.size() + 2);
    out.push_back(L'"');
    for (wchar_t ch : text) out.push_back(ch == L'"' ? L'\'' : ch);
    out.push_back(L'"');
    return out;
}

inline std::string drain(HANDLE pipe) {
    std::string bytes;
    std::array<char, 128> chunk{};
    for (;;) {
        DWORD got = 0;
        if (!::ReadFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size()), &got, nullptr)) {
            if (::GetLastError() == ERROR_BROKEN_PIPE) break;
            break;
        }
        if (got == 0) break;
        bytes.append(chunk.data(), chunk.data() + got);
    }
    return bytes;
}

struct attribute_list {
    std::vector<unsigned char> storage;
    LPPROC_THREAD_ATTRIBUTE_LIST ptr = nullptr;

    bool init(HANDLE* handles, DWORD count, std::string& error) {
        SIZE_T bytes = 0;
        (void)::InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
        storage.assign(bytes, 0);
        ptr = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        if (!::InitializeProcThreadAttributeList(ptr, 1, 0, &bytes)) {
            error = last_error("InitializeProcThreadAttributeList");
            ptr = nullptr;
            return false;
        }
        if (!::UpdateProcThreadAttribute(ptr, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                handles, sizeof(HANDLE) * count, nullptr, nullptr)) {
            error = last_error("UpdateProcThreadAttribute");
            ::DeleteProcThreadAttributeList(ptr);
            ptr = nullptr;
            return false;
        }
        return true;
    }

    ~attribute_list() {
        if (ptr) ::DeleteProcThreadAttributeList(ptr);
    }
};
#else
class fd_owner {
public:
    fd_owner() = default;
    explicit fd_owner(int value) : value_(value) {}
    fd_owner(const fd_owner&) = delete;
    fd_owner& operator=(const fd_owner&) = delete;
    fd_owner(fd_owner&& other) noexcept : value_(std::exchange(other.value_, -1)) {}
    fd_owner& operator=(fd_owner&& other) noexcept {
        if (this != &other) close(std::exchange(other.value_, -1));
        return *this;
    }
    ~fd_owner() { close(); }

    int get() const { return value_; }
    explicit operator bool() const { return value_ >= 0; }
    void close(int next = -1) {
        if (value_ >= 0) (void)::close(value_);
        value_ = next;
    }

private:
    int value_ = -1;
};

inline std::string errno_text(std::string where) {
    where += ": ";
    where += std::to_string(errno);
    return where;
}

inline bool wait_for_signal(int fd, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (left.count() < 0) left = std::chrono::milliseconds{0};
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(fd, &reads);
        timeval tv{static_cast<long>(left.count() / 1000), static_cast<long>((left.count() % 1000) * 1000)};
        const int rc = ::select(fd + 1, &reads, nullptr, nullptr, &tv);
        if (rc > 0) {
            std::uint64_t value = 0;
            return ::read(fd, &value, sizeof(value)) == sizeof(value);
        }
        if (rc == 0) return false;
        if (errno != EINTR) return false;
    }
}

inline std::string read_all(int fd) {
    std::string bytes;
    std::array<char, 128> chunk{};
    for (;;) {
        const ssize_t got = ::read(fd, chunk.data(), chunk.size());
        if (got > 0) bytes.append(chunk.data(), chunk.data() + got);
        else if (got == -1 && errno == EINTR) continue;
        else break;
    }
    return bytes;
}
#endif

} // namespace good_detail

inline ProcessResult run_process_ipc(const std::filesystem::path& executable, std::string_view payload,
                                     const std::filesystem::path& witness_path,
                                     ChildMode mode, std::chrono::milliseconds timeout) {
    ProcessResult result;
    if (payload.size() > max_payload) {
        result.error = "payload too large";
        return result;
    }

#ifdef _WIN32
    SECURITY_ATTRIBUTES inheritable{};
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;

    good_detail::win_handle pipe_read;
    good_detail::win_handle pipe_write;
    if (!::CreatePipe(pipe_read.receive(), pipe_write.receive(), &inheritable, 0)) {
        result.error = good_detail::last_error("CreatePipe");
        return result;
    }
    (void)::SetHandleInformation(pipe_read.get(), HANDLE_FLAG_INHERIT, 0);

    good_detail::win_handle shared{::CreateFileMappingW(INVALID_HANDLE_VALUE, &inheritable, PAGE_READWRITE,
        0, static_cast<DWORD>(sizeof(SharedBlock)), nullptr)};
    if (!shared) {
        result.error = good_detail::last_error("CreateFileMappingW");
        return result;
    }
    auto* block = static_cast<SharedBlock*>(::MapViewOfFile(shared.get(), FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
    if (!block) {
        result.error = good_detail::last_error("MapViewOfFile");
        return result;
    }
    prepare_shared(*block, payload);

    good_detail::win_handle ready{::CreateEventW(&inheritable, TRUE, FALSE, nullptr)};
    if (!ready) {
        (void)::UnmapViewOfFile(block);
        result.error = good_detail::last_error("CreateEventW");
        return result;
    }

    HANDLE child_handles[] = {pipe_write.get(), shared.get(), ready.get()};
    std::string attr_error;
    good_detail::attribute_list attrs;
    if (!attrs.init(child_handles, static_cast<DWORD>(std::size(child_handles)), attr_error)) {
        (void)::UnmapViewOfFile(block);
        result.error = attr_error;
        return result;
    }

    const std::wstring exe = executable.wstring();
    const std::string mode_text = mode_name(mode);
    std::wstring command = good_detail::quoted(exe) + L" --c07-child-process " +
        std::wstring(mode_text.begin(), mode_text.end()) + L" " +
        std::to_wstring(reinterpret_cast<std::uintptr_t>(shared.get())) + L" " +
        std::to_wstring(reinterpret_cast<std::uintptr_t>(ready.get())) + L" " +
        std::to_wstring(reinterpret_cast<std::uintptr_t>(pipe_write.get())) + L" " +
        good_detail::quoted(witness_path.wstring());
    std::vector<wchar_t> mutable_command(command.begin(), command.end());
    mutable_command.push_back(L'\0');

    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(si);
    si.lpAttributeList = attrs.ptr;
    PROCESS_INFORMATION pi{};
    if (!::CreateProcessW(exe.c_str(), mutable_command.data(), nullptr, nullptr, TRUE,
            EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr, nullptr, &si.StartupInfo, &pi)) {
        (void)::UnmapViewOfFile(block);
        result.error = good_detail::last_error("CreateProcessW");
        return result;
    }

    good_detail::win_handle child_process{pi.hProcess};
    good_detail::win_handle child_thread{pi.hThread};
    pipe_write.close();
    result.child_id = static_cast<std::uint64_t>(::GetProcessId(child_process.get()));

    const auto ms = static_cast<DWORD>(std::min<std::int64_t>(timeout.count(), INFINITE - 1));
    if (::WaitForSingleObject(ready.get(), ms) == WAIT_TIMEOUT &&
        ::WaitForSingleObject(child_process.get(), 0) != WAIT_OBJECT_0) {
        result.timed_out = true;
        (void)::TerminateProcess(child_process.get(), 124);
    }
    (void)::WaitForSingleObject(child_process.get(), INFINITE);
    result.child_reaped = true;

    DWORD exit_code = 0;
    (void)::GetExitCodeProcess(child_process.get(), &exit_code);
    result.exit_code = static_cast<int>(exit_code);
    result.pipe_frame = good_detail::drain(pipe_read.get());
    if (block->state == shared_done) {
        result.shared_payload.assign(reinterpret_cast<const char*>(block->output.data()),
            reinterpret_cast<const char*>(block->output.data()) + block->size);
    }
    (void)::UnmapViewOfFile(block);
#else
    const auto name = std::filesystem::temp_directory_path() /
        ("c07_l06_good_shared_" + std::to_string(::getpid()) + ".bin");
    good_detail::fd_owner shared{::open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600)};
    if (!shared) {
        result.error = good_detail::errno_text("open shared file");
        return result;
    }
    (void)::unlink(name.c_str());
    if (::ftruncate(shared.get(), static_cast<off_t>(sizeof(SharedBlock))) != 0) {
        result.error = good_detail::errno_text("ftruncate");
        return result;
    }
    auto* block = static_cast<SharedBlock*>(::mmap(nullptr, sizeof(SharedBlock), PROT_READ | PROT_WRITE,
        MAP_SHARED, shared.get(), 0));
    if (block == MAP_FAILED) {
        result.error = good_detail::errno_text("mmap");
        return result;
    }
    prepare_shared(*block, payload);

    good_detail::fd_owner ready{::eventfd(0, 0)};
    int pipe_pair[2] = {-1, -1};
    if (!ready || ::pipe(pipe_pair) != 0) {
        (void)::munmap(block, sizeof(SharedBlock));
        result.error = good_detail::errno_text("eventfd/pipe");
        return result;
    }
    good_detail::fd_owner pipe_read{pipe_pair[0]};
    good_detail::fd_owner pipe_write{pipe_pair[1]};

    const std::string exe = executable.string();
    const std::string mode_text = mode_name(mode);
    const std::string shared_arg = std::to_string(shared.get());
    const std::string ready_arg = std::to_string(ready.get());
    const std::string pipe_arg = std::to_string(pipe_write.get());
    const std::string witness_arg = witness_path.string();
    std::array<char*, 8> argv = {const_cast<char*>(exe.c_str()), const_cast<char*>("--c07-child-process"),
        const_cast<char*>(mode_text.c_str()), const_cast<char*>(shared_arg.c_str()),
        const_cast<char*>(ready_arg.c_str()), const_cast<char*>(pipe_arg.c_str()),
        const_cast<char*>(witness_arg.c_str()), nullptr};

    const pid_t child = ::fork();
    if (child == -1) {
        (void)::munmap(block, sizeof(SharedBlock));
        result.error = good_detail::errno_text("fork");
        return result;
    }
    if (child == 0) {
        (void)::close(pipe_read.get());
        ::execv(exe.c_str(), argv.data());
        ::_exit(127);
    }

    result.child_id = static_cast<std::uint64_t>(child);
    pipe_write.close();
    int status = 0;
    if (!good_detail::wait_for_signal(ready.get(), timeout)) {
        const pid_t state = ::waitpid(child, &status, WNOHANG);
        if (state == 0) {
            result.timed_out = true;
            (void)::kill(child, SIGKILL);
            while (::waitpid(child, &status, 0) == -1 && errno == EINTR) {}
        }
    }

    result.pipe_frame = good_detail::read_all(pipe_read.get());
    if (!result.timed_out) {
        while (::waitpid(child, &status, 0) == -1 && errno == EINTR) {}
    }
    result.child_reaped = true;
    if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) result.exit_code = 128 + WTERMSIG(status);
    if (block->state == shared_done) {
        result.shared_payload.assign(reinterpret_cast<const char*>(block->output.data()),
            reinterpret_cast<const char*>(block->output.data()) + block->size);
    }
    (void)::munmap(block, sizeof(SharedBlock));
#endif

    result.ok = mode == ChildMode::normal && result.exit_code == 17 && result.child_reaped && !result.timed_out;
    if (!result.ok) {
        if (result.timed_out) result.error = "child timeout";
        else if (!result.child_reaped) result.error = "child was not reaped";
        else if (result.exit_code != 17) result.error = "child exit mismatch";
        else result.error = "ipc exchange failed";
    }
    return result;
}

} // namespace c07_l06
