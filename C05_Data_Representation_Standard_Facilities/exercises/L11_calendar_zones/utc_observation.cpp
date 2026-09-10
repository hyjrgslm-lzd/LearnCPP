#include <check.hpp>

#include <c05/time.hpp>

#include <chrono>
#include <string>

int main()
{
    using namespace std::chrono;

    static_assert(year{2024}.is_leap());
    static_assert(!year{2023}.is_leap());
    static_assert(year_month_day{year{2024}, month{2}, day{29}}.ok());
    static_assert(!year_month_day{year{2023}, month{2}, day{29}}.ok());

    constexpr sys_days epoch = 1970y / January / 1;
    constexpr sys_days next = 1970y / January / 2;
    static_assert(next - epoch == days{1});

    const sys_time<milliseconds> last_ms{milliseconds{253402300799999LL}};
    const auto d = floor<days>(last_ms);
    const year_month_day ymd{d};
    check(static_cast<int>(ymd.year()) == 9999, "max manifest timestamp stays inside year 9999");
    check(static_cast<unsigned>(ymd.month()) == 12, "max manifest timestamp month");
    check(static_cast<unsigned>(ymd.day()) == 31, "max manifest timestamp day");

    auto formatted = c05::format_timestamp(0, "UTC");
    check(formatted && formatted->starts_with("1970-01-01 00:00:00.000 UTC +00:00"), "formats unix epoch in UTC");
    auto out_of_range = c05::format_timestamp(253402300800000LL, "UTC");
    check(!out_of_range && out_of_range.error().code == c05::Errc::out_of_range, "rejects timestamp outside manifest range");
    auto empty_zone = c05::format_timestamp(0, "");
    check(!empty_zone && empty_zone.error().code == c05::Errc::invalid_value, "rejects empty display zone");
}
