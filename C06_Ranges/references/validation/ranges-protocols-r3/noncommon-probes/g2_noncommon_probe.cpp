#include <check.hpp>
#include <my_transform_view.hpp>

#include <ranges>
#include <sstream>
#include <type_traits>

int main() {
    std::istringstream empty_input{""};
    auto empty = c06_g2::my_transform(std::views::istream<int>(empty_input), [](int x) { return x + 1; });
    static_assert(std::ranges::input_range<decltype(empty)>);
    static_assert(!std::ranges::common_range<decltype(empty)>);
    static_assert(!std::copyable<std::ranges::iterator_t<decltype(empty)>>);
    check(empty.begin() == empty.end(), "g2 empty input reaches sentinel");

    std::istringstream short_input{"4"};
    auto short_view = c06_g2::my_transform(std::views::istream<int>(short_input), [](int x) { return x * 3; });
    auto it = short_view.begin();
    check(*it == 12, "g2 short input transforms first value");
    it++;
    check(it == short_view.end(), "g2 postfix increment reaches sentinel");
}