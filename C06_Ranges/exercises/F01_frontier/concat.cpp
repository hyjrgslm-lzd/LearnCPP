#include <frontier_status.hpp>
#include <array>
#include <ranges>
#include <vector>
#include <algorithm>

int main() {
#if defined(__cpp_lib_ranges_concat) && __cpp_lib_ranges_concat >= 202403L
    std::vector<int> first{1, 2};
    std::array<int, 1> second{3};
    std::vector<int> empty;
    auto joined = std::views::concat(empty, first, second);
    check(std::ranges::equal(joined, std::vector{1, 2, 3}), "concat crosses empty and nonempty boundaries");
    *joined.begin() = 9;
    check(first.front() == 9, "compatible lvalue references still alias their owner");
    auto owned = std::views::concat(std::vector{4}, std::vector{5, 6});
    check(std::ranges::equal(owned, std::vector{4, 5, 6}), "temporary containers are owned by their adapted views");
    return verified("concat");
#else
    return unavailable("concat", "requires __cpp_lib_ranges_concat >= 202403L");
#endif
}
