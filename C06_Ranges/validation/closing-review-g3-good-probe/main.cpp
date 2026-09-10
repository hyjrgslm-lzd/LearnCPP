#include <my_enumerate_view.hpp>

#include <memory>
#include <ranges>
#include <vector>

int main() {
    std::vector<int> values{1, 2};
    auto base = values | std::views::transform([](int value) {
        return std::make_unique<int>(value);
    });
    auto enumerated = c06_g3::my_enumerate(base);
    auto [index, ptr] = *enumerated.begin();
    return index == 0 && *ptr == 1 ? 0 : 1;
}
