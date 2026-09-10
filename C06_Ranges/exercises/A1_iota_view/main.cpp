#include <check.hpp>
#include <iostream>
#include <iterator>
#include <ranges>
#include <vector>

int main() {
    auto bounded = std::views::iota(0, 10);
    auto unbounded = std::views::iota(0);

    static_assert(std::ranges::sized_range<decltype(bounded)>);
    static_assert(std::ranges::common_range<decltype(bounded)>);
    static_assert(std::ranges::random_access_range<decltype(bounded)>);
    static_assert(std::ranges::borrowed_range<decltype(bounded)>);

    static_assert(!std::ranges::sized_range<decltype(unbounded)>);
    static_assert(!std::ranges::common_range<decltype(unbounded)>);
    static_assert(std::ranges::random_access_range<decltype(unbounded)>);
    static_assert(std::same_as<decltype(unbounded.end()), std::unreachable_sentinel_t>);

    check(std::ranges::size(bounded) == 10, "bounded iota reports its distance");
    check(*(bounded.begin() + 4) == 4, "iota iterator supports random access");

    std::vector<int> evens;
    for (int value : unbounded | std::views::filter([](int x) { return x % 2 == 0; }) | std::views::take(5)) {
        evens.push_back(value);
    }
    check((evens == std::vector<int>{0, 2, 4, 6, 8}), "take gives finite consumption for an unbounded source");

    int raw[] = {7, 8, 9};
    auto wrapped = std::ranges::subrange(std::begin(raw), std::end(raw));
    static_assert(std::ranges::borrowed_range<decltype(wrapped)>);
    check(std::ranges::distance(wrapped) == 3, "subrange keeps an iterator/sentinel pair as a range");

    std::cout << "A1 iota/subrange checks passed\n";
}
