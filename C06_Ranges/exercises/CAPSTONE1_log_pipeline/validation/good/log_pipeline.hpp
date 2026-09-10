#pragma once

#include <algorithm>
#include <array>
#include <iterator>
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
    std::array<std::string_view, 4> fields{};
    for (auto& field : fields) {
        auto comma = line.find(',');
        if (&field == &fields.back()) {
            if (comma != std::string_view::npos) return std::nullopt;
            field = line;
        } else {
            if (comma == std::string_view::npos) return std::nullopt;
            field = line.substr(0, comma);
            line.remove_prefix(comma + 1);
        }
    }

    auto level = parse_level(fields[1]);
    if (!level) return std::nullopt;

    return LogRecord{std::string{fields[0]}, *level, std::string{fields[2]}, std::string{fields[3]}};
}

template<CountMode mode>
inline std::vector<LogRecord> collect_records_loop_impl(std::span<const std::string_view> lines,
                                                        OperationCounts* counts) {
    if constexpr (mode == CountMode::counted) {
        *counts = {};
    }
    std::vector<LogRecord> records;
    for (auto line : lines) {
        if constexpr (mode == CountMode::counted) {
            ++counts->parse_attempts;
        }
        if (auto record = parse_line(line)) {
            records.emplace_back(std::move(*record));
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
    auto parsed = lines | std::views::transform([counts](std::string_view line) {
        if constexpr (mode == CountMode::counted) {
            ++counts->parse_attempts;
        }
        return parse_line(line);
    }) | std::ranges::to<std::vector<std::optional<LogRecord>>>();

    return parsed
         | std::views::filter(&std::optional<LogRecord>::has_value)
         | std::views::transform([](std::optional<LogRecord> record) {
               return std::move(*record);
           })
         | std::ranges::to<std::vector<LogRecord>>();
}

inline std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view> lines,
                                                          OperationCounts& counts) {
    return collect_records_ranges_once_impl<CountMode::counted>(lines, &counts);
}

inline std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view> lines) {
    return collect_records_ranges_once_impl<CountMode::uncounted>(lines, nullptr);
}

inline std::map<Level, int> count_levels(std::span<const LogRecord> records) {
    auto count = [&](Level level) {
        return static_cast<int>(std::ranges::count_if(records, [=](Level current) {
            return current == level;
        }, &LogRecord::level));
    };
    return {{Level::info, count(Level::info)}, {Level::warn, count(Level::warn)}, {Level::error, count(Level::error)}};
}

inline std::map<std::string, int> count_users(std::span<const LogRecord> records) {
    std::map<std::string, int> users;
    for (const auto& record : records) ++users[record.user_id];
    return users;
}

inline PipelineReport summarize(std::span<const std::string_view> lines, int error_limit) {
    if (error_limit < 0) {
        throw std::invalid_argument("error_limit must be non-negative");
    }
    OperationCounts counts;
    auto records = collect_records_ranges_once(lines, counts);
    std::ranges::sort(records, {}, &LogRecord::timestamp);
    auto errors_view = records | std::views::filter([](const LogRecord& record) {
        return record.level == Level::error;
    }) | std::views::take(error_limit);

    auto errors = errors_view | std::ranges::to<std::vector<LogRecord>>();
    return {records, count_levels(records), count_users(records), errors, counts};
}

inline OperationCounts count_reparse_demo(std::span<const std::string_view> lines) {
    OperationCounts counts;
    auto parse_counted = [&](std::string_view line) {
        ++counts.parse_attempts;
        return parse_line(line);
    };
    auto reparsed = lines | std::views::transform(parse_counted)
                  | std::views::filter([](const std::optional<LogRecord>& record) {
                        return record.has_value();
                    })
                  | std::views::transform([](std::optional<LogRecord> record) {
                        return std::move(*record);
                    });
    for (auto record : reparsed) (void)record;
    return counts;
}

} // namespace capstone1
