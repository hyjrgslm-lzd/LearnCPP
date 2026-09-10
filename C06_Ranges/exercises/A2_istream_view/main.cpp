#include <algorithm>
#include <check.hpp>
#include <iostream>
#include <iterator>
#include <ranges>
#include <sstream>
#include <vector>

int main() {
    std::istringstream input{"1 2 3 4 5"};
    auto numbers = std::views::istream<int>(input);

    static_assert(std::ranges::input_range<decltype(numbers)>);
    static_assert(!std::ranges::forward_range<decltype(numbers)>);
    static_assert(!std::ranges::sized_range<decltype(numbers)>);
    static_assert(!std::ranges::common_range<decltype(numbers)>);
    static_assert(!std::copyable<std::ranges::iterator_t<decltype(numbers)>>);

    auto first = numbers.begin();
    check(*first == 1, "istream_view begin reads the first token");
    ++first;
    check(*first == 2, "increment advances the stream cursor");

    std::vector<int> rest;
    for (; first != std::default_sentinel; ++first) {
        rest.push_back(*first);
    }
    check((rest == std::vector<int>{2, 3, 4, 5}), "the same input iterator consumes from the current stream position");

    std::istringstream second{"10 20 30"};
    auto collected = std::ranges::to<std::vector<int>>(std::views::istream<int>(second));
    check((collected == std::vector<int>{10, 20, 30}), "ranges::to materializes an input range once");

    std::cout << "A2 istream_view single-pass checks passed\n";
}
