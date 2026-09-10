#include <process_ipc.hpp>
#include <include/process_ipc_contract.hpp>
#include <check.hpp>

#include <array>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

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
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#endif

namespace {

std::optional<c07_l06::ChildMode> parse_mode(std::string_view value) {
    if (value == "normal") return c07_l06::ChildMode::normal;
    if (value == "short-frame") return c07_l06::ChildMode::short_frame;
    if (value == "close-pipe") return c07_l06::ChildMode::close_pipe;
    if (value == "abnormal-exit") return c07_l06::ChildMode::abnormal_exit;
    if (value == "sleep") return c07_l06::ChildMode::sleep;
    return std::nullopt;
}

std::string transform(std::span<const unsigned char> input) {
    std::string out;
    out.reserve(input.size());
    for (auto it = input.rbegin(); it != input.rend(); ++it) {
        out.push_back(static_cast<char>(*it ^ 0xA7u));
    }
    return out;
}

std::string hex(std::string_view bytes) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (unsigned char ch : bytes) {
        out.push_back(alphabet[ch >> 4u]);
        out.push_back(alphabet[ch & 0x0fu]);
    }
    return out;
}

std::string make_frame(std::string_view payload) {
    std::string frame = "C07P";
    const auto size = static_cast<std::uint32_t>(payload.size());
    frame.push_back(static_cast<char>(size & 0xffu));
    frame.push_back(static_cast<char>((size >> 8u) & 0xffu));
    frame.push_back(static_cast<char>((size >> 16u) & 0xffu));
    frame.push_back(static_cast<char>((size >> 24u) & 0xffu));
    frame += payload;
    return frame;
}

bool validate_frame(std::string_view frame, std::string_view expected) {
    if (frame.size() != 8 + expected.size()) return false;
    if (frame.substr(0, 4) != "C07P") return false;
    const auto size = static_cast<std::uint32_t>(
        static_cast<std::uint32_t>(static_cast<unsigned char>(frame[4])) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(frame[5])) << 8u) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(frame[6])) << 16u) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(frame[7])) << 24u));
    return size == expected.size() && frame.substr(8) == expected;
}

std::uint64_t parse_u64(const char* text) {
    return static_cast<std::uint64_t>(std::stoull(text));
}

std::uint64_t current_process_id() {
#ifdef _WIN32
    return static_cast<std::uint64_t>(::GetCurrentProcessId());
#else
    return static_cast<std::uint64_t>(::getpid());
#endif
}

bool write_all_file(const std::filesystem::path& path, std::string_view text) {
    std::ofstream out(path, std::ios::binary);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(out);
}

#ifndef _WIN32
bool write_all_fd(int fd, const void* data, std::size_t size) {
    auto* next = static_cast<const unsigned char*>(data);
    while (size != 0) {
        const ssize_t n = ::write(fd, next, size);
        if (n > 0) {
            next += n;
            size -= static_cast<std::size_t>(n);
            continue;
        }
        if (n == -1 && errno == EINTR) continue;
        return false;
    }
    return true;
}
#endif

int child_main(int argc, char** argv) {
    if (argc < 7 || std::string_view(argv[1]) != "--c07-child-process") return 64;
    const auto mode = parse_mode(argv[2]);
    if (!mode) return 65;
    if (*mode == c07_l06::ChildMode::sleep) {
#ifdef _WIN32
        ::Sleep(30000);
#else
        ::sleep(30);
#endif
        return 66;
    }
    if (*mode == c07_l06::ChildMode::abnormal_exit) return 42;

#ifdef _WIN32
    auto* block = static_cast<c07_l06::SharedBlock*>(::MapViewOfFile(
        reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(parse_u64(argv[3]))),
        FILE_MAP_ALL_ACCESS, 0, 0, sizeof(c07_l06::SharedBlock)));
    const HANDLE event_handle = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(parse_u64(argv[4])));
    const HANDLE pipe = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(parse_u64(argv[5])));
    const std::filesystem::path witness = std::filesystem::path{std::wstring{std::filesystem::path{argv[6]}.wstring()}};
#else
    auto* block = static_cast<c07_l06::SharedBlock*>(::mmap(nullptr, sizeof(c07_l06::SharedBlock), PROT_READ | PROT_WRITE,
        MAP_SHARED, static_cast<int>(parse_u64(argv[3])), 0));
    const int event_handle = static_cast<int>(parse_u64(argv[4]));
    const int pipe = static_cast<int>(parse_u64(argv[5]));
    const std::filesystem::path witness = argv[6];
#endif
    if (
#ifdef _WIN32
        block == nullptr
#else
        block == MAP_FAILED
#endif
    ) {
        return 68;
    }
    if (block->magic != c07_l06::shared_magic || block->size > c07_l06::max_payload ||
        block->checksum != c07_l06::checksum(std::string_view{reinterpret_cast<const char*>(block->input.data()), block->size})) {
        return 69;
    }
    const auto answer = transform(std::span<const unsigned char>{block->input.data(), block->size});
    std::memcpy(block->output.data(), answer.data(), answer.size());
    block->state = c07_l06::shared_done;
    const auto witness_text = "pid=" + std::to_string(current_process_id()) + "\nhex=" + hex(answer) + "\n";
    if (!write_all_file(witness, witness_text)) return 72;
    if (*mode != c07_l06::ChildMode::close_pipe) {
        const auto frame = *mode == c07_l06::ChildMode::short_frame ? std::string{"C"} : make_frame(answer);
#ifdef _WIN32
        DWORD written = 0;
        if (!::WriteFile(pipe, frame.data(), static_cast<DWORD>(frame.size()), &written, nullptr) || written != frame.size()) return 70;
#else
        if (!write_all_fd(pipe, frame.data(), frame.size())) return 70;
#endif
    }
#ifdef _WIN32
    if (!::SetEvent(event_handle)) return 71;
    (void)::UnmapViewOfFile(block);
#else
    const std::uint64_t one = 1;
    if (!write_all_fd(event_handle, &one, sizeof(one))) {
        (void)::munmap(block, sizeof(c07_l06::SharedBlock));
        return 71;
    }
    (void)::munmap(block, sizeof(c07_l06::SharedBlock));
#endif
    return 17;
}

std::filesystem::path witness_path(std::string_view name) {
    return std::filesystem::temp_directory_path() /
        ("c07_l06_" + std::to_string(current_process_id()) + "_" + std::string{name} + ".witness");
}

bool witness_matches(const std::filesystem::path& path, std::uint64_t child_id, std::string_view expected) {
    std::ifstream in(path, std::ios::binary);
    const std::string text{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    return text.find("pid=" + std::to_string(child_id) + "\n") != std::string::npos &&
           text.find("hex=" + hex(expected) + "\n") != std::string::npos &&
           child_id != current_process_id();
}

void remove_if_exists(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view(argv[1]) == "--c07-child-process") {
        return child_main(argc, argv);
    }

    const auto self = std::filesystem::absolute(argv[0]);
    const std::string payload = "unknown payload: \x01\x02 process ipc bytes";
    const auto expected = transform(std::span<const unsigned char>{reinterpret_cast<const unsigned char*>(payload.data()), payload.size()});
    const auto timeout = std::chrono::milliseconds{1500};

    const auto normal_witness = witness_path("normal");
    remove_if_exists(normal_witness);
    const auto ok = c07_l06::run_process_ipc(self, payload, normal_witness, c07_l06::ChildMode::normal, timeout);
    check(ok.exit_code == 17 && ok.child_reaped && !ok.timed_out, "child exit and reap observed");
    check(witness_matches(normal_witness, ok.child_id, expected), "child witness proves spawned fixture");
    check(validate_frame(ok.pipe_frame, expected), "child payload mismatch");
    check(ok.shared_payload == expected, "shared payload matches child output");
    remove_if_exists(normal_witness);

    const auto bad_path = c07_l06::run_process_ipc(self.parent_path() / "missing-child-executable", payload,
        witness_path("bad_path"), c07_l06::ChildMode::normal, timeout);
    check(!bad_path.ok && (!bad_path.child_id || bad_path.child_reaped), "bad child path is reported");

    const auto short_witness = witness_path("short");
    remove_if_exists(short_witness);
    const auto short_frame = c07_l06::run_process_ipc(self, payload, short_witness, c07_l06::ChildMode::short_frame, timeout);
    check(short_frame.child_reaped && witness_matches(short_witness, short_frame.child_id, expected), "short frame child witness observed");
    check(!validate_frame(short_frame.pipe_frame, expected), "short frame is rejected after child reap");
    remove_if_exists(short_witness);

    const auto eof_witness = witness_path("eof");
    remove_if_exists(eof_witness);
    const auto eof = c07_l06::run_process_ipc(self, payload, eof_witness, c07_l06::ChildMode::close_pipe, timeout);
    check(eof.child_reaped && witness_matches(eof_witness, eof.child_id, expected), "peer close child witness observed");
    check(eof.pipe_frame.empty(), "peer close produces EOF");
    remove_if_exists(eof_witness);

    const auto abnormal = c07_l06::run_process_ipc(self, payload, witness_path("abnormal"), c07_l06::ChildMode::abnormal_exit, timeout);
    check(!abnormal.ok && abnormal.child_reaped && abnormal.exit_code != 17, "abnormal child exit is bounded");

    const auto slow = c07_l06::run_process_ipc(self, payload, witness_path("slow"), c07_l06::ChildMode::sleep, std::chrono::milliseconds{200});
    check(!slow.ok && slow.timed_out && slow.child_reaped, "timeout kills and reaps only the child");

    std::cout << "L06 process IPC checks passed\n";
}
