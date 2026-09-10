#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

inline std::optional<Level> parse_level(std::string_view text) {
    if (text == "INFO") return Level::info;
    if (text == "WARN") return Level::warn;
    if (text == "ERROR") return Level::error;
    return std::nullopt;
}

inline std::optional<LogRecord> parse_line(std::string_view line) {
    const auto first = line.find(',');
    if (first == std::string_view::npos) return std::nullopt;
    const auto second = line.find(',', first + 1);
    if (second == std::string_view::npos) return std::nullopt;
    const auto third = line.find(',', second + 1);
    if (third == std::string_view::npos) return std::nullopt;
    if (line.find(',', third + 1) != std::string_view::npos) return std::nullopt;

    auto level = parse_level(line.substr(first + 1, second - first - 1));
    if (!level) return std::nullopt;

    return LogRecord{
        std::string{line.substr(0, first)},
        *level,
        std::string{line.substr(second + 1, third - second - 1)},
        std::string{line.substr(third + 1)},
    };
}

template<CountMode mode>
inline std::vector<LogRecord> collect_records_loop_impl(std::span<const std::string_view> lines,
                                                        OperationCounts* counts) {
    if constexpr (mode == CountMode::counted) {
        *counts = {};
    }
    std::vector<LogRecord> records;
    for (std::string_view line : lines) {
        if constexpr (mode == CountMode::counted) {
            ++counts->parse_attempts;
        }
        if (auto parsed = parse_line(line)) {
            records.push_back(std::move(*parsed));
        }
    }
    return records;
}

inline std::vector<LogRecord> collect_records_loop(std::span<const std::string_view> lines,
                                                   OperationCounts& counts) {
    return collect_records_loop_impl<CountMode::counted>(lines, &counts);
}

inline std::vector<LogRecord> collect_records_loop(std::span<const std::string_view> lines) {
    return collect_records_loop_impl<CountMode::uncounted>(lines, nullptr);
}

template<CountMode mode>
inline std::vector<LogRecord> collect_records_ranges_once_impl(std::span<const std::string_view> lines,
                                                               OperationCounts* counts) {
    if constexpr (mode == CountMode::counted) {
        *counts = {};
    }
    auto parse_counted = [counts](std::string_view line) {
        if constexpr (mode == CountMode::counted) {
            ++counts->parse_attempts;
        }
        return parse_line(line);
    };
    auto records = lines | std::views::transform(parse_counted)
                | std::views::filter([](const std::optional<LogRecord>& record) {
                      return record.has_value();
                  })
                | std::views::transform([](std::optional<LogRecord> record) {
                      return std::move(*record);
                  });
    return records | std::ranges::to<std::vector<LogRecord>>();
}

inline std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view> lines,
                                                          OperationCounts& counts) {
    return collect_records_ranges_once_impl<CountMode::counted>(lines, &counts);
}

inline std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view> lines) {
    return collect_records_ranges_once_impl<CountMode::uncounted>(lines, nullptr);
}

inline std::map<Level, int> count_by_level(std::span<const LogRecord> records) {
    return {
        {Level::info, static_cast<int>(std::ranges::count_if(records, std::bind_front(std::ranges::equal_to{}, Level::info), &LogRecord::level))},
        {Level::warn, static_cast<int>(std::ranges::count_if(records, std::bind_front(std::ranges::equal_to{}, Level::warn), &LogRecord::level))},
        {Level::error, static_cast<int>(std::ranges::count_if(records, std::bind_front(std::ranges::equal_to{}, Level::error), &LogRecord::level))},
    };
}

inline std::map<std::string, int> count_by_user(std::span<const LogRecord> records) {
    std::map<std::string, int> users;
    for (const auto& record : records) {
        ++users[record.user_id];
    }
    return users;
}

inline PipelineReport summarize(std::span<const std::string_view> lines, int error_limit) {
    if (error_limit < 0) {
        throw std::invalid_argument("error_limit must be non-negative");
    }
    OperationCounts counts;
    auto records = collect_records_ranges_once(lines, counts);
    std::ranges::sort(records, std::less{}, &LogRecord::timestamp);
    auto errors = records | std::views::filter([](const LogRecord& record) {
        return record.level == Level::error;
    }) | std::views::take(error_limit);
    auto first_errors = errors | std::ranges::to<std::vector<LogRecord>>();

    return PipelineReport{records, count_by_level(records), count_by_user(records), first_errors, counts};
}

inline OperationCounts count_reparse_demo(std::span<const std::string_view> lines) {
    OperationCounts counts;
    auto parse_counted = [&counts](std::string_view line) {
        ++counts.parse_attempts;
        return parse_line(line);
    };
    auto records = lines | std::views::transform(parse_counted)
                | std::views::filter([](const std::optional<LogRecord>& record) {
                      return record.has_value();
                  })
                | std::views::transform([](std::optional<LogRecord> record) {
                      return std::move(*record);
                  });
    for (auto record : records) {
        (void)record;
    }
    return counts;
}

} // namespace capstone1
