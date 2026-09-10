#pragma once
#include <include/process_ipc_contract.hpp>

namespace c07_l06 {

inline ProcessResult run_process_ipc(const std::filesystem::path&, std::string_view,
                                     const std::filesystem::path&,
                                     ChildMode, std::chrono::milliseconds) {
    return {.ok = false, .pipe_frame = "", .shared_payload = "",
            .error = "student TODO: create the child, exchange bytes, wait, and reap",
            .exit_code = -1, .timed_out = false, .child_reaped = false, .child_id = 0};
}

} // namespace c07_l06
