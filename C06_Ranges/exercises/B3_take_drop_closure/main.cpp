#include <check.hpp>
#include <iostream>
#include <list>
#include <ranges>
#include <vector>

int main() {
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    auto taken = data | std::views::take(3);
    auto dropped = data | std::views::drop(2);

    static_assert(std::ranges::sized_range<decltype(taken)>);
    static_assert(std::ranges::common_range<decltype(taken)>);
    static_assert(std::random_access_iterator<decltype(taken.begin())>);
    static_assert(std::ranges::sized_range<decltype(dropped)>);
    static_assert(std::random_access_iterator<decltype(dropped.begin())>);

    check((std::ranges::to<std::vector<int>>(taken) == std::vector<int>{0, 1, 2}), "take keeps a prefix");
    check((std::ranges::to<std::vector<int>>(dropped) == std::vector<int>{2, 3, 4, 5}), "drop removes a prefix");

    auto dropped_while = data | std::views::drop_while([](int x) { return x < 3; });
    check((std::ranges::to<std::vector<int>>(dropped_while) == std::vector<int>{3, 4, 5}), "drop_while removes while the predicate is true");

    std::list<int> linked{0, 1, 2, 3};
    auto linked_drop = linked | std::views::drop(1);
    static_assert(std::bidirectional_iterator<decltype(linked_drop.begin())>);
    static_assert(!std::random_access_iterator<decltype(linked_drop.begin())>);

    auto unbounded = std::views::iota(0) | std::views::take_while([](int x) { return x < 4; });
    static_assert(!std::ranges::sized_range<decltype(unbounded)>);
    static_assert(!std::ranges::common_range<decltype(unbounded)>);
    check((std::ranges::to<std::vector<int>>(unbounded) == std::vector<int>{0, 1, 2, 3}), "take_while stops by predicate");

    auto closure = std::views::drop(1) | std::views::take(3);
    auto piped = data | closure;
    check((std::ranges::to<std::vector<int>>(piped) == std::vector<int>{1, 2, 3}), "range adaptor closures compose left to right");

    std::cout << "B3 take/drop/closure checks passed\n";
}
