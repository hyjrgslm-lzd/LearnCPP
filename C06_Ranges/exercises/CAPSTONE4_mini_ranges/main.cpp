#include <check.hpp>
#include <my_ranges/06_consumers.hpp>

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <vector>

namespace capstone4 {

void run_mini_ranges_checks() {
    static_assert(!std::is_function_v<decltype(my::ranges::begin)>);
    static_assert(my::ranges::range<std::vector<int>>);
    static_assert(!my::ranges::view<std::vector<int>>);
    static_assert(my::ranges::input_range<std::vector<int>>);
    static_assert(my::ranges::forward_range<std::vector<int>>);
    static_assert(my::ranges::view<my::views::iota_view<int>>);
    static_assert(my::ranges::enable_borrowed_range<my::views::iota_view<int>>);
    static_assert(my::ranges::view<my::views::single_view<int>>);
    static_assert(std::is_same_v<decltype(my::views::single_view<int>{42}.begin()), int*>);

    auto taken = my::views::take_view{my::views::iota_view{0, 100}, 5};
    static_assert(!std::is_same_v<decltype(taken.begin()), decltype(taken.end())>);

    using transformed = decltype(my::views::iota_view{0, 10} | my::views::transform([](int x) { return x * x; }));
    using transformed_iter = my::ranges::iterator_t<transformed>;
    static_assert(std::is_same_v<typename transformed_iter::iterator_concept, std::random_access_iterator_tag>);

    int arr[] = {1, 2, 3};
    check(my::ranges::begin(arr) == arr, "begin CPO supports arrays");
    check(my::ranges::iter_move(arr) == 1, "iter_move fallback moves dereferenced iterator");

    auto v = my::views::iota(1, 11)
           | my::views::transform([](int x) { return x * x; })
           | my::views::take(5)
           | my::ranges::to<std::vector<int>>();
    check(v == std::vector<int>({1, 4, 9, 16, 25}), "take_view stops after requested count");

    auto short_take = my::views::iota(1, 4) | my::views::take(99) | my::ranges::to<std::vector<int>>();
    check(short_take == std::vector<int>({1, 2, 3}), "take_view also stops at base end");

    auto my_view = my::views::iota(1, 6) | my::views::transform([](int x) { return x * 2; });
    std::vector<int> out;
    std::ranges::copy(my_view, std::back_inserter(out));
    check(out == std::vector<int>({2, 4, 6, 8, 10}), "stdlib ranges can consume mini-ranges view");
}

} // namespace capstone4

int main() {
    capstone4::run_mini_ranges_checks();
}
