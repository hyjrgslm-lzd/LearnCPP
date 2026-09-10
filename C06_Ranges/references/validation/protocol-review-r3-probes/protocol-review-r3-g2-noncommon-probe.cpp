#include <my_transform_view.hpp>
#include <ranges>
#include <sstream>
#include <type_traits>

int main() {
    std::istringstream input{"1 2"};
    auto transformed = c06_g2::my_transform(std::views::istream<int>(input), [](int x) { return x * 2; });
    static_assert(std::ranges::input_range<decltype(transformed)>);
    static_assert(!std::ranges::common_range<decltype(transformed)>);
    static_assert(!std::copyable<std::ranges::iterator_t<decltype(transformed)>>);
    int sum = 0;
    for (int value : transformed) sum += value;
    return sum == 6 ? 0 : 1;
}
