#include <check.hpp>
#include <iostream>
#include <ranges>
#include <vector>

int main() {
    std::vector<std::vector<int>> nested{{1, 2, 3}, {4, 5}, {}, {6}};
    auto flat = nested | std::views::join;
    static_assert(std::bidirectional_iterator<decltype(flat.begin())>);
    static_assert(!std::random_access_iterator<decltype(flat.begin())>);
    static_assert(std::same_as<decltype(flat.begin()), decltype(flat.end())>);
    static_assert(std::ranges::common_range<decltype(flat)>);

    check((std::ranges::to<std::vector<int>>(flat) == std::vector<int>{1, 2, 3, 4, 5, 6}), "join flattens stored inner ranges");

    auto non_common = std::views::iota(0)
        | std::views::filter([](int x) { return x % 2 == 0; })
        | std::views::take(4);
    static_assert(!std::same_as<decltype(non_common.begin()), decltype(non_common.end())>);
    static_assert(!std::ranges::common_range<decltype(non_common)>);
    check((std::ranges::to<std::vector<int>>(non_common) == std::vector<int>{0, 2, 4, 6}), "non-common iter/sentinel ranges are a separate case from stored join");

    auto generated_inner = std::views::iota(1, 4)
        | std::views::transform([](int n) { return std::views::iota(0, n); })
        | std::views::join;
    check((std::ranges::to<std::vector<int>>(generated_inner) == std::vector<int>{0, 0, 1, 0, 1, 2}), "join can flatten prvalue inner views");

    std::vector<std::vector<char>> words{{'a', 'b'}, {'c'}, {'d', 'e'}};
    auto joined_with = words | std::views::join_with('-');
    check((std::ranges::to<std::vector<char>>(joined_with) == std::vector<char>{'a', 'b', '-', 'c', '-', 'd', 'e'}), "join_with inserts delimiters between inner ranges");

    std::cout << "C1_1 join checks passed\n";
}
