#pragma once
#include <c07/completion_io.hpp>
#include <expected>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

namespace c07_l08 {
namespace detail {

struct request_state {
    std::size_t expected_bytes = 0;
    bool completed = false;
};

inline bool target_error_allowed(const std::error_code& error) {
    return !error || error == std::errc::operation_canceled
#ifdef _WIN32
        || (error.category() == std::system_category() && error.value() == ERROR_OPERATION_ABORTED)
#endif
        ;
}

inline bool cancel_error_allowed(const std::error_code& error) {
    return !error || error == std::errc::no_such_process
#ifdef _WIN32
        || (error.category() == std::system_category() && error.value() == ERROR_NOT_FOUND)
#endif
        ;
}

inline std::unexpected<std::error_code> protocol_error() {
    return std::unexpected(std::make_error_code(std::errc::protocol_error));
}

} // namespace detail

inline std::expected<void, std::error_code> validate_completion_ledger(const c07::completion_probe_report& report) {
    std::unordered_map<std::uint64_t, detail::request_state> requests;
    std::unordered_map<std::uint64_t, std::uint64_t> cancels;
    std::unordered_set<std::uint64_t> completed_cancels;
    bool payload_read_completed = false;

    for (const auto& event : report.events) {
        switch (event.kind) {
        case c07::completion_event_kind::read_accepted:
            if (event.request_id == 0 || event.target_request_id != 0 || event.error
                || requests.contains(event.request_id) || cancels.contains(event.request_id)) {
                return detail::protocol_error();
            }
            requests.emplace(event.request_id, detail::request_state{event.bytes, false});
            break;
        case c07::completion_event_kind::read_completed: {
            auto found = requests.find(event.request_id);
            if (found == requests.end() || found->second.completed || event.target_request_id != 0
                || event.error || event.bytes != found->second.expected_bytes) {
                return detail::protocol_error();
            }
            found->second.completed = true;
            if (event.bytes == report.payload.size()) payload_read_completed = true;
            break;
        }
        case c07::completion_event_kind::target_completed: {
            auto found = requests.find(event.request_id);
            if (found == requests.end() || found->second.completed || event.target_request_id != 0
                || !detail::target_error_allowed(event.error) || event.bytes > found->second.expected_bytes) {
                return detail::protocol_error();
            }
            found->second.completed = true;
            break;
        }
        case c07::completion_event_kind::cancel_submitted:
            if (event.request_id == 0 || event.target_request_id == 0 || event.error
                || !requests.contains(event.target_request_id) || requests.contains(event.request_id)
                || cancels.contains(event.request_id)) {
                return detail::protocol_error();
            }
            cancels.emplace(event.request_id, event.target_request_id);
            break;
        case c07::completion_event_kind::cancel_completed: {
            auto found = cancels.find(event.request_id);
            if (found == cancels.end() || found->second != event.target_request_id
                || completed_cancels.contains(event.request_id) || !detail::cancel_error_allowed(event.error)) {
                return detail::protocol_error();
            }
            completed_cancels.insert(event.request_id);
            break;
        }
        }
    }

    if (!payload_read_completed) return detail::protocol_error();
    for (const auto& [request_id, state] : requests) {
        (void)request_id;
        if (!state.completed) return detail::protocol_error();
    }
    for (const auto& [cancel_id, target_id] : cancels) {
        (void)target_id;
        if (!completed_cancels.contains(cancel_id)) return detail::protocol_error();
    }
    return {};
}

} // namespace c07_l08
