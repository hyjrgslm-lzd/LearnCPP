#pragma once
#include <chrono>
#include <cmath>
#include <expected>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>

#include <c05/types.hpp>

namespace c05 {
namespace detail {
inline std::string format_offset(std::chrono::seconds offset)
{
    auto total = offset.count();
    const auto sign = total < 0 ? '-' : '+';
    if (total < 0) {
        total = -total;
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << sign << std::setw(2) << std::setfill('0') << (total / 3600) << ':'
        << std::setw(2) << std::setfill('0') << ((total / 60) % 60);
    if (total % 60 != 0) {
        out << ':' << std::setw(2) << std::setfill('0') << (total % 60);
    }
    return out.str();
}

inline std::string format_ymdhms(std::chrono::sys_time<std::chrono::milliseconds> tp,
    std::string_view zone, std::chrono::seconds offset)
{
    const auto day = std::chrono::floor<std::chrono::days>(tp);
    const std::chrono::year_month_day ymd{day};
    const std::chrono::hh_mm_ss tod{tp - day};
    if (!ymd.ok() || ymd.year() < std::chrono::year{1} || ymd.year() > std::chrono::year{9999}) {
        return {};
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setw(4) << std::setfill('0') << static_cast<int>(ymd.year()) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.month()) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.day()) << ' '
        << std::setw(2) << std::setfill('0') << tod.hours().count() << ':'
        << std::setw(2) << std::setfill('0') << tod.minutes().count() << ':'
        << std::setw(2) << std::setfill('0') << tod.seconds().count() << '.'
        << std::setw(3) << std::setfill('0') << tod.subseconds().count() << ' '
        << zone << ' ' << format_offset(offset);
    return out.str();
}
}

inline std::expected<std::string, DataError> format_timestamp(std::int64_t millis, std::string_view zone = "UTC")
{
    if (millis < min_timestamp_ms || millis > max_timestamp_ms) {
        return std::unexpected(DataError{Errc::out_of_range, 0, OffsetUnit::byte, "modified_at"});
    }
    if (zone.empty()) {
        return std::unexpected(DataError{Errc::invalid_value, 0, OffsetUnit::byte, "display_zone"});
    }
    const std::chrono::sys_time<std::chrono::milliseconds> utc{std::chrono::milliseconds{millis}};
    if (zone == "UTC") {
        return detail::format_ymdhms(utc, zone, std::chrono::seconds{0});
    }
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    const std::chrono::time_zone* tz = nullptr;
    try {
        tz = std::chrono::get_tzdb().locate_zone(std::string(zone));
    } catch (const std::runtime_error&) {
        return std::unexpected(DataError{Errc::io_error, 0, OffsetUnit::byte, "display_zone"});
    }
    const auto info = tz->get_info(utc);
    const auto local = std::chrono::sys_time<std::chrono::milliseconds>{utc.time_since_epoch() + info.offset};
    auto formatted = detail::format_ymdhms(local, zone, info.offset);
    if (formatted.empty()) {
        return std::unexpected(DataError{Errc::out_of_range, 0, OffsetUnit::byte, "modified_at"});
    }
    return formatted;
#else
    return std::unexpected(DataError{Errc::io_error, 0, OffsetUnit::byte, "display_zone"});
#endif
}
}
