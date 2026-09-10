#include <array>
#include <check.hpp>
#include <iostream>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

int main() {
    static_assert(std::ranges::range<std::vector<int>>);
    static_assert(std::ranges::range<std::string_view>);
    static_assert(std::ranges::range<std::array<int, 3>>);
    static_assert(!std::ranges::range<int>);

    static_assert(std::ranges::view<std::string_view>);
    static_assert(std::ranges::view<std::span<int>>);
    static_assert(std::ranges::view<std::ranges::iota_view<int, int>>);
    static_assert(!std::ranges::view<std::vector<int>>);

    static_assert(!std::ranges::common_range<std::ranges::iota_view<int, std::unreachable_sentinel_t>>);
    static_assert(std::ranges::borrowed_range<std::string_view>);
    static_assert(std::ranges::borrowed_range<std::span<int>>);
    static_assert(!std::ranges::borrowed_range<std::vector<int>>);

    std::vector<int> data{1, 2, 3};
    std::span<int> view{data};
    auto borrowed = std::ranges::find(view, 2);
    auto dangling = std::ranges::find(std::vector<int>{1, 2, 3}, 2);

    static_assert(std::same_as<decltype(dangling), std::ranges::dangling>);
    check(borrowed != view.end(), "span returns a usable iterator");
    check(*borrowed == 2, "borrowed iterator points at the matched element");

    std::vector<int> out;
    for (int value : std::views::iota(0) | std::views::take(3)) {
        out.push_back(value);
    }
    check((out == std::vector<int>{0, 1, 2}), "take bounds an infinite iota");

    std::cout << "mental model: range/view/sentinel/borrowed checks passed\n";
}
