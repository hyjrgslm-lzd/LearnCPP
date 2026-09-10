#pragma once
#include <c07/completion_io.hpp>
#include <expected>
#include <system_error>
#include <unordered_map>

namespace c07_l08 {

inline bool c07_l08_target_status_ok(const std::error_code& error) {
    return !error || error == std::errc::operation_canceled
#ifdef _WIN32
        || (error.category() == std::system_category() && error.value() == ERROR_OPERATION_ABORTED)
#endif
        ;
}

inline bool c07_l08_cancel_status_ok(const std::error_code& error) {
    return !error || error == std::errc::no_such_process
#ifdef _WIN32
        || (error.category() == std::system_category() && error.value() == ERROR_NOT_FOUND)
#endif
        ;
}

inline std::expected<void, std::error_code> validate_completion_ledger(const c07::completion_probe_report& report) {
    struct request {
        std::size_t expected = 0;
        int completions = 0;
    };
    struct cancel {
        std::uint64_t target = 0;
        bool done = false;
    };

    std::unordered_map<std::uint64_t, request> requests;
    std::unordered_map<std::uint64_t, cancel> cancels;
    bool saw_payload = false;

    for (const auto& event : report.events) {
        if (event.kind == c07::completion_event_kind::read_accepted) {
            if (event.request_id == 0 || event.target_request_id != 0 || event.error
                || requests.contains(event.request_id) || cancels.contains(event.request_id)) {
                return std::unexpected(std::make_error_code(std::errc::protocol_error));
            }
            requests.emplace(event.request_id, request{event.bytes, 0});
        } else if (event.kind == c07::completion_event_kind::read_completed) {
            auto iter = requests.find(event.request_id);
            if (iter == requests.end() || iter->second.completions != 0 || event.target_request_id != 0
                || event.error || event.bytes != iter->second.expected) {
                return std::unexpected(std::make_error_code(std::errc::protocol_error));
            }
            ++iter->second.completions;
            saw_payload = saw_payload || event.bytes == report.payload.size();
        } else if (event.kind == c07::completion_event_kind::target_completed) {
            auto iter = requests.find(event.request_id);
            if (iter == requests.end() || iter->second.completions != 0 || event.target_request_id != 0
                || event.bytes > iter->second.expected || !c07_l08_target_status_ok(event.error)) {
                return std::unexpected(std::make_error_code(std::errc::protocol_error));
            }
            ++iter->second.completions;
        } else if (event.kind == c07::completion_event_kind::cancel_submitted) {
            if (event.request_id == 0 || event.target_request_id == 0 || event.error
                || !requests.contains(event.target_request_id) || requests.contains(event.request_id)
                || cancels.contains(event.request_id)) {
                return std::unexpected(std::make_error_code(std::errc::protocol_error));
            }
            cancels.emplace(event.request_id, cancel{event.target_request_id, false});
        } else if (event.kind == c07::completion_event_kind::cancel_completed) {
            auto iter = cancels.find(event.request_id);
            if (iter == cancels.end() || iter->second.done || iter->second.target != event.target_request_id
                || !c07_l08_cancel_status_ok(event.error)) {
                return std::unexpected(std::make_error_code(std::errc::protocol_error));
            }
            iter->second.done = true;
        }
    }

    if (!saw_payload) return std::unexpected(std::make_error_code(std::errc::protocol_error));
    for (const auto& [id, request] : requests) {
        (void)id;
        if (request.completions != 1) return std::unexpected(std::make_error_code(std::errc::protocol_error));
    }
    for (const auto& [id, cancel] : cancels) {
        (void)id;
        if (!cancel.done) return std::unexpected(std::make_error_code(std::errc::protocol_error));
    }
    return {};
}

} // namespace c07_l08
