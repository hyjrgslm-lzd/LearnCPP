#pragma once
#include <include/process_ipc_contract.hpp>

#include <algorithm>
#include <array>
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
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace c07_l06 {
namespace detail {

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
    HANDLE* put() { reset(); return &handle_; }
    explicit operator bool() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }
    void reset(HANDLE next = nullptr) {
        if (*this) (void)::CloseHandle(handle_);
        handle_ = next;
    }
private:
    HANDLE handle_ = nullptr;
};

inline std::string win_error(const char* where) {
    return std::string{where} + ": " + std::to_string(::GetLastError());
}

inline std::wstring quote(std::wstring value) {
    std::wstring out = L"\"";
    for (wchar_t ch : value) out += ch == L'"' ? L'\'' : ch;
    out += L"\"";
    return out;
}

inline std::string read_pipe(HANDLE pipe) {
    std::string out;
    std::array<char, 256> buffer{};
    for (;;) {
        DWORD read = 0;
        if (!::ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)) {
            if (::GetLastError() == ERROR_BROKEN_PIPE) break;
            break;
        }
        if (read == 0) break;
        out.append(buffer.data(), buffer.data() + read);
    }
    return out;
}
#else
class unique_fd {
public:
    unique_fd() = default;
    explicit unique_fd(int fd) : fd_(fd) {}
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;
    unique_fd(unique_fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    unique_fd& operator=(unique_fd&& other) noexcept {
        if (this != &other) reset(std::exchange(other.fd_, -1));
        return *this;
    }
    ~unique_fd() { reset(); }
    int get() const { return fd_; }
    int* put() { reset(); return &fd_; }
    explicit operator bool() const { return fd_ >= 0; }
    void reset(int next = -1) {
        if (fd_ >= 0) (void)::close(fd_);
        fd_ = next;
    }
private:
    int fd_ = -1;
};

inline std::string posix_error(const char* where) {
    return std::string{where} + ": " + std::to_string(errno);
}

inline bool wait_eventfd(int fd, std::chrono::milliseconds timeout) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeval tv{static_cast<long>(timeout.count() / 1000), static_cast<long>((timeout.count() % 1000) * 1000)};
    for (;;) {
        const int rc = ::select(fd + 1, &set, nullptr, nullptr, &tv);
        if (rc > 0) {
            std::uint64_t value = 0;
            return ::read(fd, &value, sizeof(value)) == sizeof(value);
        }
        if (rc == -1 && errno == EINTR) continue;
        return false;
    }
}

inline std::string read_fd(int fd) {
    std::string out;
    std::array<char, 256> buffer{};
    for (;;) {
        const ssize_t n = ::read(fd, buffer.data(), buffer.size());
        if (n > 0) out.append(buffer.data(), buffer.data() + n);
        else if (n == -1 && errno == EINTR) continue;
        else break;
    }
    return out;
}
#endif

} // namespace detail

inline ProcessResult run_process_ipc(const std::filesystem::path& executable, std::string_view payload,
                                     const std::filesystem::path& witness_path,
                                     ChildMode mode, std::chrono::milliseconds timeout) {
    ProcessResult result;
    if (payload.size() > max_payload) {
        result.error = "payload too large";
        return result;
    }

#ifdef _WIN32
    SECURITY_ATTRIBUTES inherit{};
    inherit.nLength = sizeof(inherit);
    inherit.bInheritHandle = TRUE;

    detail::unique_handle read_pipe;
    detail::unique_handle write_pipe;
    if (!::CreatePipe(read_pipe.put(), write_pipe.put(), &inherit, 0)) {
        result.error = detail::win_error("CreatePipe");
        return result;
    }
    (void)::SetHandleInformation(read_pipe.get(), HANDLE_FLAG_INHERIT, 0);

    detail::unique_handle mapping{::CreateFileMappingW(INVALID_HANDLE_VALUE, &inherit, PAGE_READWRITE, 0,
        static_cast<DWORD>(sizeof(SharedBlock)), nullptr)};
    if (!mapping) {
        result.error = detail::win_error("CreateFileMappingW");
        return result;
    }
    auto* block = static_cast<SharedBlock*>(::MapViewOfFile(mapping.get(), FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
    if (!block) {
        result.error = detail::win_error("MapViewOfFile");
        return result;
    }
    prepare_shared(*block, payload);

    detail::unique_handle done{::CreateEventW(&inherit, TRUE, FALSE, nullptr)};
    if (!done) {
        (void)::UnmapViewOfFile(block);
        result.error = detail::win_error("CreateEventW");
        return result;
    }

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    SIZE_T bytes = 0;
    (void)::InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    std::vector<unsigned char> storage(bytes);
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!::InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &bytes)) {
        (void)::UnmapViewOfFile(block);
        result.error = detail::win_error("InitializeProcThreadAttributeList");
        return result;
    }
    HANDLE inherit_list[] = {write_pipe.get(), mapping.get(), done.get()};
    if (!::UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            inherit_list, sizeof(inherit_list), nullptr, nullptr)) {
        ::DeleteProcThreadAttributeList(startup.lpAttributeList);
        (void)::UnmapViewOfFile(block);
        result.error = detail::win_error("UpdateProcThreadAttribute");
        return result;
    }

    const auto app = executable.wstring();
    const auto mode_text = mode_name(mode);
    const auto witness_text = witness_path.wstring();
    std::wstring command = detail::quote(app) + L" --c07-child-process " +
        std::wstring(mode_text.begin(), mode_text.end()) + L" " +
        std::to_wstring(reinterpret_cast<std::uintptr_t>(mapping.get())) + L" " +
        std::to_wstring(reinterpret_cast<std::uintptr_t>(done.get())) + L" " +
        std::to_wstring(reinterpret_cast<std::uintptr_t>(write_pipe.get())) + L" " + detail::quote(witness_text);
    std::vector<wchar_t> mutable_command(command.begin(), command.end());
    mutable_command.push_back(L'\0');
    PROCESS_INFORMATION pi{};
    if (!::CreateProcessW(app.c_str(), mutable_command.data(), nullptr, nullptr, TRUE,
            EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr, nullptr,
            &startup.StartupInfo, &pi)) {
        ::DeleteProcThreadAttributeList(startup.lpAttributeList);
        (void)::UnmapViewOfFile(block);
        result.error = detail::win_error("CreateProcessW");
        return result;
    }
    ::DeleteProcThreadAttributeList(startup.lpAttributeList);
    detail::unique_handle process{pi.hProcess};
    detail::unique_handle thread{pi.hThread};
    result.child_id = static_cast<std::uint64_t>(::GetProcessId(process.get()));
    write_pipe.reset();

    const auto wait_ms = static_cast<DWORD>(std::min<std::int64_t>(timeout.count(), INFINITE - 1));
    DWORD wait = ::WaitForSingleObject(done.get(), wait_ms);
    if (wait == WAIT_TIMEOUT && ::WaitForSingleObject(process.get(), 0) != WAIT_OBJECT_0) {
        result.timed_out = true;
        (void)::TerminateProcess(process.get(), 124);
    }
    (void)::WaitForSingleObject(process.get(), INFINITE);
    result.child_reaped = true;
    DWORD code = 0;
    (void)::GetExitCodeProcess(process.get(), &code);
    result.exit_code = static_cast<int>(code);
    result.pipe_frame = detail::read_pipe(read_pipe.get());
    if (block->state == shared_done) {
        result.shared_payload.assign(reinterpret_cast<const char*>(block->output.data()),
            reinterpret_cast<const char*>(block->output.data()) + block->size);
    }
    (void)::UnmapViewOfFile(block);
#else
    const auto backing = std::filesystem::temp_directory_path() / ("c07_l06_shared_" + std::to_string(::getpid()) + ".bin");
    detail::unique_fd shared{::open(backing.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600)};
    if (!shared) {
        result.error = detail::posix_error("open");
        return result;
    }
    (void)::unlink(backing.c_str());
    if (::ftruncate(shared.get(), static_cast<off_t>(sizeof(SharedBlock))) != 0) {
        result.error = detail::posix_error("ftruncate");
        return result;
    }
    auto* block = static_cast<SharedBlock*>(::mmap(nullptr, sizeof(SharedBlock), PROT_READ | PROT_WRITE,
        MAP_SHARED, shared.get(), 0));
    if (block == MAP_FAILED) {
        result.error = detail::posix_error("mmap");
        return result;
    }
    prepare_shared(*block, payload);
    detail::unique_fd event_fd{::eventfd(0, 0)};
    int pipefd[2] = {-1, -1};
    if (!event_fd || ::pipe(pipefd) != 0) {
        (void)::munmap(block, sizeof(SharedBlock));
        result.error = detail::posix_error("eventfd/pipe");
        return result;
    }
    detail::unique_fd read_pipe{pipefd[0]};
    detail::unique_fd write_pipe{pipefd[1]};

    const std::string exe = executable.string();
    const std::string mode_text = mode_name(mode);
    const std::string shared_text = std::to_string(shared.get());
    const std::string event_text = std::to_string(event_fd.get());
    const std::string pipe_text = std::to_string(write_pipe.get());
    const std::string witness_text = witness_path.string();
    std::array<char*, 8> argv{const_cast<char*>(exe.c_str()), const_cast<char*>("--c07-child-process"),
        const_cast<char*>(mode_text.c_str()), const_cast<char*>(shared_text.c_str()),
        const_cast<char*>(event_text.c_str()), const_cast<char*>(pipe_text.c_str()),
        const_cast<char*>(witness_text.c_str()), nullptr};
    const pid_t pid = ::fork();
    if (pid == -1) {
        (void)::munmap(block, sizeof(SharedBlock));
        result.error = detail::posix_error("fork");
        return result;
    }
    if (pid == 0) {
        (void)::close(read_pipe.get());
        ::execv(exe.c_str(), argv.data());
        ::_exit(127);
    }
    result.child_id = static_cast<std::uint64_t>(pid);
    write_pipe.reset();
    bool got_event = detail::wait_eventfd(event_fd.get(), timeout);
    int status = 0;
    if (!got_event) {
        const pid_t ready = ::waitpid(pid, &status, WNOHANG);
        if (ready == 0) {
            result.timed_out = true;
            (void)::kill(pid, SIGKILL);
            (void)::waitpid(pid, &status, 0);
        }
    }
    result.pipe_frame = detail::read_fd(read_pipe.get());
    if (!result.timed_out) {
        while (::waitpid(pid, &status, 0) == -1 && errno == EINTR) {}
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

    result.ok = mode == ChildMode::normal && result.exit_code == 17 && !result.timed_out && result.child_reaped;
    if (!result.ok) {
        if (result.timed_out) result.error = "child timeout";
        else if (result.exit_code != 17) result.error = "child exit mismatch";
        else result.error = "process ipc failed";
    }
    return result;
}

} // namespace c07_l06
