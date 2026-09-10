#include <algorithm>
#include <check.hpp>
#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

struct MinimalContainer {
    std::vector<int> data;

    void push_back(int value) {
        data.push_back(value);
    }

    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};

int main() {
    auto odds = std::views::iota(1, 11)
        | std::views::filter([](int x) { return x % 2 == 1; })
        | std::ranges::to<std::vector<int>>();
    check((odds == std::vector<int>{1, 3, 5, 7, 9}), "pipeline ranges::to materializes filtered values");

    auto called = std::ranges::to<std::vector<int>>(std::views::iota(1, 4));
    check((called == std::vector<int>{1, 2, 3}), "function-call ranges::to materializes a range");

    auto deduced = std::views::iota(1, 4) | std::ranges::to<std::vector>();
    static_assert(std::same_as<decltype(deduced), std::vector<int>>);
    check(deduced == called, "CTAD form deduces vector element type from the range");

    auto as_list = std::views::iota(1, 4) | std::ranges::to<std::list<int>>();
    check(std::ranges::equal(as_list, std::vector<int>{1, 2, 3}), "ranges::to targets other sequence containers");

    auto as_string = std::views::iota('a', char{'d'}) | std::ranges::to<std::string>();
    check(as_string == "abc", "ranges::to can build a string from char values");

    auto tokens = std::string_view{"alpha,beta,gamma"}
        | std::views::split(',')
        | std::views::transform([](auto part) { return part | std::ranges::to<std::string>(); })
        | std::ranges::to<std::vector>();
    check((tokens == std::vector<std::string>{"alpha", "beta", "gamma"}), "split subranges can be materialized into vector<string>");

    auto minimal = std::views::iota(1, 4) | std::ranges::to<MinimalContainer>();
    check((minimal.data == std::vector<int>{1, 2, 3}), "fallback insertion path calls push_back");

    std::vector<int> from_range{std::from_range, std::views::iota(1, 4)};
    check(from_range == called, "from_range_t is a construction tag, ranges::to is a consuming adaptor");
    static_assert(std::is_same_v<decltype(std::from_range), const std::from_range_t>);

    std::cout << "C2_3 ranges::to checks passed\n";
}
