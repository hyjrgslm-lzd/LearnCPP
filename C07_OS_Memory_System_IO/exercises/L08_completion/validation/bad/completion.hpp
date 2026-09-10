#pragma once
#include <c07/completion_io.hpp>
#include <expected>
#include <system_error>

namespace c07_l08 {
inline std::expected<void, std::error_code> validate_completion_ledger(const c07::completion_probe_report& report) {
    for (const auto& event : report.events) {
        if (event.kind == c07::completion_event_kind::read_completed && event.bytes == report.payload.size()) return {};
    }
    return std::unexpected(std::make_error_code(std::errc::protocol_error));
}
} // namespace c07_l08
