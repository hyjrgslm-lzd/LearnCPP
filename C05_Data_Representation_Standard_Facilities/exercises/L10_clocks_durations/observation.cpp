#include <check.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <type_traits>

int main()
{
    using namespace std::chrono;

    static_assert(steady_clock::is_steady);
    static_assert(!std::is_same_v<system_clock, steady_clock>);

    constexpr milliseconds coarse = duration_cast<milliseconds>(microseconds{2500});
    static_assert(coarse == milliseconds{2});
    static_assert(floor<milliseconds>(microseconds{2500}) == milliseconds{2});
    static_assert(round<milliseconds>(microseconds{2500}) == milliseconds{2});
    static_assert(ceil<milliseconds>(microseconds{2500}) == milliseconds{3});

    constexpr auto one_day = days{1};
    static_assert(duration_cast<hours>(one_day).count() == 24);

    const auto wall_a = system_clock::now();
    const auto mono_a = steady_clock::now();
    const auto mono_b = steady_clock::now();
    check(mono_b >= mono_a, "steady_clock does not step backward inside one process observation");
    check(wall_a.time_since_epoch().count() != 0, "system_clock exposes a wall-clock epoch duration");

    using ms = duration<std::int64_t, std::milli>;
    static_assert(ms::max().count() == std::numeric_limits<std::int64_t>::max());
    check(duration_cast<milliseconds>(seconds{1}).count() == 1000, "exact duration conversion keeps value");
}
