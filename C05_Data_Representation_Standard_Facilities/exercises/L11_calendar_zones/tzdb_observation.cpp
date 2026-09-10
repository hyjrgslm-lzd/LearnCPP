#include <check.hpp>

#include <c05/time.hpp>

#include <chrono>
#include <exception>
#include <iostream>

int main()
{
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    using namespace std::chrono;
    try {
        const auto& db = get_tzdb();
        const auto* ny = db.locate_zone("America/New_York");
        const local_time<minutes> gap = local_days{2024y / March / 10} + 2h + 30min;
        const local_time<minutes> fold = local_days{2024y / November / 3} + 1h + 30min;
        check(ny->get_info(gap).result == local_info::nonexistent, "New York 2024-03-10 02:30 is nonexistent");
        check(ny->get_info(fold).result == local_info::ambiguous, "New York 2024-11-03 01:30 is ambiguous");
        const auto early = ny->to_sys(fold, choose::earliest);
        const auto late = ny->to_sys(fold, choose::latest);
        check(early != late, "ambiguous local time has two UTC instants");
        auto formatted = c05::format_timestamp(0, "America/New_York");
        check(formatted && formatted->find("America/New_York -05:00") != std::string::npos, "formats timestamp with named zone offset");
        auto historic = c05::format_timestamp(-2208988800000LL, "Asia/Kolkata");
        check(historic && historic->find("+05:21:10") != std::string::npos, "prints non-minute historical offsets with seconds");
        auto local_overflow = c05::format_timestamp(253402300799999LL, "Pacific/Kiritimati");
        check(!local_overflow && local_overflow.error().code == c05::Errc::out_of_range, "rejects local display outside manifest calendar range");
        std::cout << "tzdb version: " << db.version << '\n';
    } catch (const std::exception& e) {
        std::cout << "tzdb unavailable: " << e.what() << '\n';
        return 77;
    }
    return 0;
#else
    std::cout << "tzdb API unavailable in this standard library\n";
    return 77;
#endif
}
