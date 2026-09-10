#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace c07_l06 {

enum class ChildMode { normal, short_frame, close_pipe, abnormal_exit, sleep };

struct ProcessResult {
    bool ok = false;
    std::string pipe_frame;
    std::string shared_payload;
    std::string error;
    int exit_code = -1;
    bool timed_out = false;
    bool child_reaped = false;
    std::uint64_t child_id = 0;
};

inline constexpr std::uint32_t shared_magic = 0xC0706001u;
inline constexpr std::uint32_t shared_done = 0xC0706002u;
inline constexpr std::size_t max_payload = 128;

struct SharedBlock {
    std::uint32_t magic = shared_magic;
    std::uint32_t size = 0;
    std::uint32_t checksum = 0;
    std::uint32_t state = 0;
    std::array<unsigned char, max_payload> input{};
    std::array<unsigned char, max_payload> output{};
};

inline std::string mode_name(ChildMode mode) {
    switch (mode) {
    case ChildMode::normal: return "normal";
    case ChildMode::short_frame: return "short-frame";
    case ChildMode::close_pipe: return "close-pipe";
    case ChildMode::abnormal_exit: return "abnormal-exit";
    case ChildMode::sleep: return "sleep";
    }
    return "normal";
}

inline std::uint32_t checksum(std::string_view text) {
    std::uint32_t sum = 2166136261u;
    for (unsigned char ch : text) {
        sum ^= ch;
        sum *= 16777619u;
    }
    return sum;
}

inline void prepare_shared(SharedBlock& block, std::string_view payload) {
    block = {};
    block.magic = shared_magic;
    block.size = static_cast<std::uint32_t>(payload.size() > max_payload ? max_payload : payload.size());
    block.checksum = checksum(payload.substr(0, block.size));
    for (std::size_t i = 0; i != block.size; ++i) block.input[i] = static_cast<unsigned char>(payload[i]);
}

ProcessResult run_process_ipc(const std::filesystem::path& executable, std::string_view payload,
                              const std::filesystem::path& witness_path,
                              ChildMode mode, std::chrono::milliseconds timeout);

} // namespace c07_l06
