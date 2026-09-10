#pragma once

#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace capstone1 {

enum class Level { info, warn, error };

struct LogRecord {
    std::string timestamp;
    Level level{};
    std::string user_id;
    std::string message;
};

struct OperationCounts {
    int parse_attempts{};
};

struct PipelineReport {
    std::vector<LogRecord> records;
    std::map<Level, int> level_counts;
    std::map<std::string, int> user_counts;
    std::vector<LogRecord> first_errors;
    OperationCounts counts;
};

enum class CountMode { counted, uncounted };

inline std::optional<LogRecord> parse_line(std::string_view) {
    return std::nullopt;
}

inline std::vector<LogRecord> collect_records_loop(std::span<const std::string_view>,
                                                   OperationCounts& counts) {
    counts = {};
    return {};
}

inline std::vector<LogRecord> collect_records_loop(std::span<const std::string_view>) {
    return {};
}

inline std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view>,
                                                          OperationCounts& counts) {
    counts = {};
    return {};
}

inline std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view>) {
    return {};
}

inline PipelineReport summarize(std::span<const std::string_view>, int error_limit) {
    if (error_limit < 0) {
        throw std::invalid_argument("error_limit must be non-negative");
    }
    return {};
}

inline OperationCounts count_reparse_demo(std::span<const std::string_view>) {
    return {};
}

} // namespace capstone1
