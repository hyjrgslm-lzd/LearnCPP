#include <ranges>
#include <vector>

int main() {
    std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}};
    auto flat = nested | std::views::join;
    static_assert(std::ranges::common_range<decltype(flat)>);
    static_assert(std::same_as<decltype(flat.begin()), decltype(flat.end())>);
}
