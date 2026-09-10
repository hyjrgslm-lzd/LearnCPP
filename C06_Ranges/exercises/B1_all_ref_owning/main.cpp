#include <check.hpp>
#include <iostream>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

int main() {
    std::vector<int> data{1, 2, 3};
    auto ref = std::views::all(data);
    static_assert(std::same_as<decltype(ref), std::ranges::ref_view<std::vector<int>>>);
    static_assert(std::ranges::borrowed_range<decltype(ref)>);
    check(&*ref.begin() == data.data(), "ref_view iterators point into the original owner");
    data[0] = 42;
    check(*ref.begin() == 42, "ref_view observes later mutations through the original owner");

    auto owned = std::views::all(std::vector<int>{4, 5, 6});
    static_assert(std::same_as<decltype(owned), std::ranges::owning_view<std::vector<int>>>);
    static_assert(!std::ranges::borrowed_range<decltype(owned)>);
    check(std::ranges::size(owned) == 3, "owning_view stores the moved-in container");
    check(std::accumulate(owned.begin(), owned.end(), 0) == 15, "owning_view iteration reads the owned elements");

    auto iota = std::views::iota(0, 3);
    auto iota_all = std::views::all(iota);
    static_assert(std::same_as<decltype(iota_all), decltype(iota)>);

    static_assert(std::ranges::viewable_range<decltype(data)&>);
    static_assert(std::ranges::viewable_range<std::vector<int>>);
    static_assert(std::ranges::view<std::string_view>);
    static_assert(std::ranges::view<std::span<int>>);

    std::string text = "abc";
    auto chars = std::views::all(std::string_view{text});
    check(*chars.begin() == 'a', "string_view is already a lightweight borrowed view");

    std::cout << "B1 all/ref/owning checks passed\n";
}
