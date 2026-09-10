#include <frontier_status.hpp>
#include <ranges>
#include <vector>

int main() {
#if defined(__cpp_lib_ranges_cache_latest) && __cpp_lib_ranges_cache_latest >= 202411L
    std::vector<int> input{1, 2, 3};
    int calls = 0; // Observation instrumentation; the transformation result remains pure.
    auto values = input | std::views::transform([&](int x) { ++calls; return x * 2; })
                        | std::views::cache_latest;
    static_assert(std::ranges::input_range<decltype(values)>);
    static_assert(!std::ranges::forward_range<decltype(values)>);
    auto it = values.begin();
    check(*it == 2 && *it == 2 && calls == 1, "repeated dereference reuses the latest computed value");
    ++it;
    check(*it == 4 && calls == 2, "increment invalidates the previous cache");
    ++it;
    check(*it == 6 && calls == 3, "each visited position is computed once");
    ++it;
    check(it == values.end(), "cached traversal reaches its sentinel");
    return verified("cache_latest");
#else
    return unavailable("cache_latest", "requires __cpp_lib_ranges_cache_latest >= 202411L");
#endif
}
