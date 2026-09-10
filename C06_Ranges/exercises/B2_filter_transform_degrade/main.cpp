#include <check.hpp>
#include <forward_list>
#include <iostream>
#include <ranges>
#include <vector>

int main() {
    std::vector<int> data{1, 2, 3, 4, 5, 6};
    auto all = data | std::views::all;
    auto filtered = all | std::views::filter([](int x) { return x % 2 == 1; });
    auto squared = filtered | std::views::transform([](int x) { return x * x; });
    auto transformed_first = all | std::views::transform([](int x) { return x * x; });

    static_assert(std::random_access_iterator<std::vector<int>::iterator>);
    static_assert(std::random_access_iterator<decltype(all.begin())>);
    static_assert(std::bidirectional_iterator<decltype(filtered.begin())>);
    static_assert(!std::random_access_iterator<decltype(filtered.begin())>);
    static_assert(std::bidirectional_iterator<decltype(squared.begin())>);
    static_assert(std::random_access_iterator<decltype(transformed_first.begin())>);

    std::vector<int> out;
    for (int value : squared) {
        out.push_back(value);
    }
    check((out == std::vector<int>{1, 9, 25}), "filter then transform keeps only odd squares");

    std::forward_list<int> list{1, 2, 3, 4};
    auto forward_filtered = list | std::views::filter([](int x) { return x > 1; });
    static_assert(std::forward_iterator<decltype(forward_filtered.begin())>);
    static_assert(!std::bidirectional_iterator<decltype(forward_filtered.begin())>);

    std::cout << "B2 filter/transform concept checks passed\n";
}
