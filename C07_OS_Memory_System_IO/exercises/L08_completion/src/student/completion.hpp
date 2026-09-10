#pragma once
#include <c07/completion_io.hpp>
#include <expected>
#include <system_error>

namespace c07_l08 {
inline std::expected<void, std::error_code> validate_completion_ledger(const c07::completion_probe_report&) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}
} // namespace c07_l08
