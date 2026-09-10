#include <check.hpp>
#include <forward_list>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

int main() {
    auto odds = std::views::iota(0) | std::views::filter([](int x) { return x % 2 == 1; }) | std::views::take(3);
    static_assert(!std::ranges::common_range<decltype(odds)>);
    auto common_odds = odds | std::views::common;
    static_assert(std::ranges::common_range<decltype(common_odds)>);
    check((std::vector<int>(common_odds.begin(), common_odds.end()) == std::vector<int>{1, 3, 5}), "common adapts iterator/sentinel for iterator-pair consumers");

    std::vector<int> data{1, 2, 3};
    auto reversed = data | std::views::reverse;
    static_assert(std::random_access_iterator<decltype(reversed.begin())>);
    check((std::ranges::to<std::vector<int>>(reversed) == std::vector<int>{3, 2, 1}), "reverse preserves random access for vector");
    static_assert(!std::ranges::bidirectional_range<std::forward_list<int>>);

    std::map<std::string, int> scores{{"alice", 2}, {"bob", 5}};
    auto keys = scores | std::views::keys;
    auto values = scores | std::views::values;
    check((std::ranges::to<std::vector<std::string>>(keys) == std::vector<std::string>{"alice", "bob"}), "keys projects map keys");
    check((std::ranges::to<std::vector<int>>(values) == std::vector<int>{2, 5}), "values projects map values");

    std::vector<std::tuple<int, std::string, double>> rows{{1, "one", 1.5}, {2, "two", 2.5}};
    auto names = rows | std::views::elements<1>;
    check((std::ranges::to<std::vector<std::string>>(names) == std::vector<std::string>{"one", "two"}), "elements<N> projects tuple-like elements");

    auto dangling = std::ranges::find(std::map<std::string, int>{{"x", 1}} | std::views::values, 1);
    static_assert(std::same_as<decltype(dangling), std::ranges::dangling>);

    std::cout << "C1_3 common/reverse/elements checks passed\n";
}
