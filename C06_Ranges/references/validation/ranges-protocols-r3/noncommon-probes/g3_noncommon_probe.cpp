#include <check.hpp>
#include <my_enumerate_view.hpp>

#include <ranges>
#include <sstream>
#include <type_traits>

int main() {
    std::istringstream empty_input{""};
    auto empty = c06_g3::my_enumerate(std::views::istream<int>(empty_input));
    static_assert(std::ranges::input_range<decltype(empty)>);
    static_assert(!std::ranges::common_range<decltype(empty)>);
    static_assert(!std::copyable<std::ranges::iterator_t<decltype(empty)>>);
    check(empty.begin() == empty.end(), "g3 empty input reaches sentinel");

    std::istringstream short_input{"9"};
    auto short_view = c06_g3::my_enumerate(std::views::istream<int>(short_input));
    auto it = short_view.begin();
    check((*it).first == 0 && (*it).second == 9, "g3 short input reads first indexed value");
    it++;
    check(it == short_view.end(), "g3 postfix increment reaches sentinel");
}