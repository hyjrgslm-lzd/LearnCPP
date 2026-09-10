#include <my_enumerate_view.hpp>
#include <ranges>
#include <sstream>

int main() {
    std::istringstream input{"1 2"};
    auto ints = std::views::istream<int>(input);
    auto enumerated = c06_g3::my_enumerate(ints);
    static_assert(std::ranges::input_range<decltype(enumerated)>);
    int sum = 0;
    for (auto [index, value] : enumerated) {
        sum += static_cast<int>(index) + value;
    }
    return sum == 4 ? 0 : 1;
}
