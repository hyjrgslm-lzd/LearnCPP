#include <frontier_status.hpp>
#include <optional>
#include <ranges>
#include <vector>
#include <algorithm>
#include <type_traits>

int main() {
#if defined(__cpp_lib_optional_range_support) && __cpp_lib_optional_range_support >= 202406L
    std::optional<int> value;
    static_assert(std::ranges::view<decltype(value)>);
    static_assert(std::ranges::contiguous_range<decltype(value)>);
    check(std::ranges::empty(value), "disengaged optional is an empty range");
    value = 9;
    auto doubled = value | std::views::transform([](int x) { return x * 2; })
        | std::ranges::to<std::vector<int>>();
    check(doubled == std::vector{18}, "engaged optional contributes one value");
    static_assert(std::is_same_v<decltype(std::ranges::find(std::optional<int>{9}, 9)), std::ranges::dangling>);
    return verified("optional range (C03 bridge)");
#else
    return unavailable("optional range", "requires __cpp_lib_optional_range_support >= 202406L");
#endif
}
