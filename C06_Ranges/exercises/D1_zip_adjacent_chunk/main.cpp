#include <check.hpp>
#include <iostream>
#include <list>
#include <numeric>
#include <ranges>
#include <tuple>
#include <vector>

int main() {
    std::vector<int> a{1, 2, 3, 4};
    std::vector<double> b{1.5, 2.5, 3.5};
    auto zipped = std::views::zip(a, b);
    static_assert(std::ranges::random_access_range<decltype(zipped)>);
    check(std::ranges::size(zipped) == 3, "zip length is the shortest input");

    double zip_sum = 0;
    for (auto [x, y] : zipped) {
        zip_sum += x + y;
    }
    check(zip_sum == 13.5, "zip visits aligned elements");

    auto sums = std::views::zip_transform(std::plus<>{}, a, b) | std::ranges::to<std::vector<double>>();
    check((sums == std::vector<double>{2.5, 4.5, 6.5}), "zip_transform maps aligned elements");

    auto adjacent = a | std::views::adjacent<3>;
    static_assert(std::tuple_size_v<std::ranges::range_value_t<decltype(adjacent)>> == 3);
    check(std::ranges::size(adjacent) == 2, "adjacent<3> makes overlapping triples");
    int adjacent_sum = 0;
    for (auto [x, y, z] : adjacent) {
        adjacent_sum += x + y + z;
    }
    check(adjacent_sum == 15, "adjacent<3> windows can be consumed with structured bindings");

    auto slide = a | std::views::slide(3);
    check(std::ranges::size(slide) == 2, "slide(3) has the same window count as adjacent<3>");

    auto chunks = a | std::views::chunk(3);
    std::vector<int> chunk_sizes;
    for (auto chunk : chunks) {
        chunk_sizes.push_back(static_cast<int>(std::ranges::size(chunk)));
    }
    check((chunk_sizes == std::vector<int>{3, 1}), "chunk makes non-overlapping groups");

    auto stride = a | std::views::stride(2);
    static_assert(std::ranges::random_access_range<decltype(stride)>);
    check((std::ranges::to<std::vector<int>>(stride) == std::vector<int>{1, 3}), "stride skips by a fixed step");

    std::list<int> linked{1, 2, 3, 4};
    auto linked_stride = linked | std::views::stride(2);
    static_assert(!std::ranges::random_access_range<decltype(linked_stride)>);

    std::cout << "D1 zip/adjacent/chunk/stride checks passed\n";
}
