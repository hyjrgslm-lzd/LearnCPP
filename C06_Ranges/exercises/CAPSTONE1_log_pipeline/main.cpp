#include <check.hpp>
#include <log_pipeline.hpp>
#include <log_pipeline_data.hpp>

#include <algorithm>
#include <iostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

using capstone1::Level;
using capstone1::LogRecord;

constexpr auto keep_all = [](std::string_view) { return true; };

bool same_record(const LogRecord& left, const LogRecord& right) {
    return left.timestamp == right.timestamp
        && left.level == right.level
        && left.user_id == right.user_id
        && left.message == right.message;
}

void check_record(const LogRecord& record,
                  std::string_view timestamp,
                  Level level,
                  std::string_view user_id,
                  std::string_view message) {
    check(record.timestamp == timestamp, "parsed timestamp matches input");
    check(record.level == level, "parsed level matches input");
    check(record.user_id == user_id, "parsed user_id matches input");
    check(record.message == message, "parsed message matches input");
}

void check_parse_line() {
    auto parsed = capstone1::parse_line("2024-01-01T00:00:03,ERROR,alice,connection refused");
    check(parsed.has_value(), "valid row parses");
    check_record(*parsed, "2024-01-01T00:00:03", Level::error, "alice", "connection refused");

    check(!capstone1::parse_line("INVALID_LINE_NO_COMMAS"), "line without four fields is rejected");
    check(!capstone1::parse_line("2024-01-01T00:00:07,BADLEVEL,dave,unknown"), "unknown level is rejected");
    check(!capstone1::parse_line("a,INFO,b,c,extra"), "extra comma is rejected");
}

void check_collectors(std::span<const std::string_view> lines) {
    capstone1::OperationCounts loop_counts;
    auto loop_records = capstone1::collect_records_loop(lines, loop_counts);
    check(loop_counts.parse_attempts == static_cast<int>(lines.size()), "loop baseline parses each input line once");
    check(loop_records.size() == 7, "invalid rows are filtered out");

    capstone1::OperationCounts range_counts;
    auto range_records = capstone1::collect_records_ranges_once(lines, range_counts);
    check(range_counts.parse_attempts == static_cast<int>(lines.size()),
          "valid records must parse each input line once");
    check(range_records.size() == loop_records.size(), "ranges collector keeps all valid rows");
    check(std::ranges::equal(loop_records, range_records, same_record), "ranges collector matches loop baseline");
}

void check_report(std::span<const std::string_view> lines) {
    auto report = capstone1::summarize(lines, 3);
    check(report.records.size() == 7, "summary keeps seven valid records");
    check(report.counts.parse_attempts == static_cast<int>(lines.size()),
          "summary parser count stays single pass");
    check(report.level_counts[Level::info] == 2, "INFO count is two");
    check(report.level_counts[Level::warn] == 2, "WARN count is two");
    check(report.level_counts[Level::error] == 3, "ERROR count is three");
    check(report.user_counts["alice"] == 3, "alice has three valid rows");
    check(report.user_counts["bob"] == 2, "bob has two valid rows");
    check(report.user_counts["carol"] == 2, "carol has two valid rows");
    check(!report.user_counts.contains("dave"), "invalid BADLEVEL row is not counted");

    check(report.first_errors.size() == 3, "first three ERROR rows are retained");
    check_record(report.first_errors[0], "2024-01-01T00:00:03", Level::error, "alice", "connection refused");
    check_record(report.first_errors[1], "2024-01-01T00:00:04", Level::error, "carol", "timeout");
    check_record(report.first_errors[2], "2024-01-01T00:00:06", Level::error, "alice", "segfault detected");

    auto zero = capstone1::summarize(lines, 0);
    check(zero.first_errors.empty(), "zero error limit returns no errors");

    auto all_errors = capstone1::summarize(lines, 99);
    check(all_errors.first_errors.size() == 3, "oversized error limit returns all matching errors");

    bool rejected_negative = false;
    try {
        (void)capstone1::summarize(lines, -1);
    } catch (const std::invalid_argument&) {
        rejected_negative = true;
    }
    check(rejected_negative, "negative error limit throws invalid_argument");
}

void check_reparse_demo(std::span<const std::string_view> lines) {
    auto counts = capstone1::count_reparse_demo(lines);
    check(counts.parse_attempts == 16, "safe transform/filter demo reparses valid rows");
}

} // namespace

int main() {
    using Raw = std::span<const std::string_view>;
    using RefPipe = decltype(std::declval<Raw>() | std::views::filter(keep_all));

    static_assert(std::ranges::borrowed_range<Raw>);
    static_assert(!std::ranges::borrowed_range<RefPipe>);
    static_assert(std::bidirectional_iterator<decltype(std::declval<RefPipe&>().begin())>);
    static_assert(!std::random_access_iterator<decltype(std::declval<RefPipe&>().begin())>);

    std::span<const std::string_view> lines{capstone1_data::sample_lines};
    check_parse_line();
    check_collectors(lines);
    check_report(lines);
    check_reparse_demo(lines);

    std::cout << "CAPSTONE1_log_pipeline checks OK\n";
}
