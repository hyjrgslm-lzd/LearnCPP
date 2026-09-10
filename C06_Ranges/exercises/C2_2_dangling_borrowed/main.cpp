#include <check.hpp>
#include <iostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

template <class T>
struct my_borrowed_span {
    T* first{};
    std::size_t count{};

    T* begin() const { return first; }
    T* end() const { return first + count; }
};

template <class T>
inline constexpr bool std::ranges::enable_borrowed_range<my_borrowed_span<T>> = true;

int main() {
    auto dangling = std::ranges::find(std::vector<int>{1, 2, 3}, 2);
    static_assert(std::same_as<decltype(dangling), std::ranges::dangling>);

    std::string backing = "abc";
    std::string_view borrowed_text{backing};
    auto found = std::ranges::find(borrowed_text, 'b');
    check(found != borrowed_text.end() && *found == 'b', "string_view is borrowed but still depends on its character owner");

    int values[] = {4, 5, 6};
    my_borrowed_span<int> mine{values, 3};
    static_assert(std::ranges::borrowed_range<my_borrowed_span<int>>);
    auto mine_found = std::ranges::find(mine, 5);
    check(mine_found == values + 1, "custom borrowed span returns a real iterator");

    static_assert(std::same_as<std::ranges::borrowed_iterator_t<std::string_view>, std::string_view::iterator>);
    static_assert(std::same_as<std::ranges::borrowed_iterator_t<std::vector<int>>, std::ranges::dangling>);
    static_assert(std::same_as<std::ranges::borrowed_subrange_t<my_borrowed_span<int>>, std::ranges::subrange<int*>>);

    std::cout << "C2_2 borrowed/dangling checks passed\n";
}
