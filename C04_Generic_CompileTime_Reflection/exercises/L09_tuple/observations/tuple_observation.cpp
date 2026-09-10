#include <check.hpp>

#include <array>
#include <iostream>
#include <tuple>
#include <utility>

template<class Tuple, class F, std::size_t... Is>
void each(Tuple&& tuple, F& f, std::index_sequence<Is...>) {
    (static_cast<void>(f(std::get<Is>(std::forward<Tuple>(tuple)))), ...);
}

int main() {
    auto tuple = std::tuple{1, 2, 3};
    int sum = std::apply([](auto... xs) { return (0 + ... + xs); }, tuple);
    check(sum == 6, "std::apply expands tuple into one call");

    std::array<int, 3> seen{};
    int i = 0;
    auto record = [&](int value) { seen[i++] = value; };
    each(tuple, record, std::make_index_sequence<3>{});
    check((seen == std::array{1, 2, 3}), "index_sequence can visit elements one by one");
    std::cout << "L09 tuple observation OK\n";
}
