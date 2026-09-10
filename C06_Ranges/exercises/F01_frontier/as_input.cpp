#include <frontier_status.hpp>
#include <ranges>
#include <span>
#include <vector>
#include <algorithm>

int main() {
#if defined(__cpp_lib_ranges_as_input) && __cpp_lib_ranges_as_input >= 202502L
    int source[]{1, 2, 3};
    auto values = std::span{source} | std::views::as_input;
    static_assert(std::ranges::input_range<decltype(values)>);
    static_assert(!std::ranges::forward_range<decltype(values)>);
    static_assert(std::ranges::borrowed_range<decltype(values)>);
    check(std::ranges::equal(values, std::vector{1, 2, 3}), "as_input preserves values while weakening traversal requirements");
    return verified("as_input (P3828 current name)");
#else
    return unavailable("as_input", "requires current-name macro >= 202502L; to_input is not silently substituted");
#endif
}
