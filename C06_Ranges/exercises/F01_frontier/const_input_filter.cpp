#include <frontier_status.hpp>
#include <ranges>
#include <span>

template<class View>
int verify_const_filter(const View& values) {
    if constexpr (std::ranges::input_range<const View>) {
        int sum = 0;
        for (int value : values) sum += value;
        check(sum == 6, "const input filter traverses matching values");
        return verified("P3725 const input filter");
    } else {
        return unavailable("P3725 const input filter", "as_input exists but the constrained const filter extension is absent");
    }
}

int main() {
#if defined(__cpp_lib_ranges_as_input) && __cpp_lib_ranges_as_input >= 202502L
    int source[]{1, 2, 3, 4};
    auto values = std::span{source} | std::views::as_input
        | std::views::filter([](int x) { return x % 2 == 0; });
    return verify_const_filter(values);
#else
    return unavailable("P3725 const input filter", "current-name as_input prerequisite is absent");
#endif
}
