#include <check.hpp>
#include <iostream>
#include <ranges>
#include <tuple>
#include <vector>

int main() {
    auto repeated = std::views::repeat(42);
    static_assert(!std::ranges::sized_range<decltype(repeated)>);
    static_assert(!std::ranges::common_range<decltype(repeated)>);
    static_assert(std::ranges::random_access_range<decltype(repeated)>);

    int total = 0;
    for (int value : repeated | std::views::take(5)) {
        total += value;
    }
    check(total == 210, "unbounded repeat must be explicitly bounded before consumption");

    auto bounded = std::views::repeat(7, 4);
    static_assert(std::ranges::sized_range<decltype(bounded)>);
    check(std::ranges::size(bounded) == 4, "bounded repeat exposes size");

    std::vector<int> nums{1, 2};
    std::vector<char> chars{'a', 'b', 'c'};
    bool flags[] = {false, true};
    auto product = std::views::cartesian_product(nums, chars, flags);
    static_assert(std::ranges::random_access_range<decltype(product)>);
    check(std::ranges::size(product) == 12, "cartesian product size is the product of sized dimensions");

    int seen = 0;
    for (auto [n, c, flag] : product) {
        seen += n + c + (flag ? 1 : 0);
    }
    check(seen == 1200, "cartesian product visits every combination once");

    auto empty_product = std::views::cartesian_product();
    check(std::ranges::size(empty_product) == 1, "zero-dimensional cartesian product has one empty tuple");
    check(std::tuple_size_v<std::ranges::range_value_t<decltype(empty_product)>> == 0, "empty product element is tuple<>");

    std::cout << "A3 repeat/cartesian checks passed\n";
}
