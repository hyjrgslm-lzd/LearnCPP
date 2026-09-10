#include <check.hpp>

#include <array>
#include <iostream>
#include <utility>

template<class... Fs>
void comma_order(Fs&&... fs) {
    (static_cast<void>(std::forward<Fs>(fs)()), ...);
}

template<auto... Values>
constexpr auto sum = (0 + ... + Values);

template<bool... Values>
constexpr bool all = (Values && ...);

template<bool... Values>
constexpr bool any = (Values || ...);

int main() {
    static_assert(all<>, "empty && fold is true");
    static_assert(!any<>, "empty || fold is false");
    static_assert(sum<1, 2, 3> == 6);

    std::array<int, 3> seen{};
    int step = 0;
    comma_order([&] { seen[0] = step++; }, [&] { seen[1] = step++; }, [&] { seen[2] = step++; });
    check((seen == std::array{0, 1, 2}), "comma fold gives an observable left-to-right order");
    std::cout << "L07 pack observation OK\n";
}
