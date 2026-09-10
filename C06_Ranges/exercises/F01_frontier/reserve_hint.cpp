#include <frontier_status.hpp>
#include <array>
#include <ranges>
#include <vector>
#include <algorithm>

int main() {
#if defined(__cpp_lib_ranges_reserve_hint) && __cpp_lib_ranges_reserve_hint >= 202502L
    struct sentinel { int* last; bool operator==(int* it) const { return it == last; } };
    struct hinted_range {
        std::array<int, 3> data{3, 1, 4};
        int* begin() { return data.data(); }
        sentinel end() { return {data.data() + data.size()}; }
        std::size_t reserve_hint() const { return 8; }
    } values;
    static_assert(std::ranges::approximately_sized_range<hinted_range>);
    static_assert(!std::ranges::sized_range<hinted_range>);
    check(std::ranges::reserve_hint(values) == 8, "hint is distinct from the exact number of elements");
    auto materialized = std::ranges::to<std::vector<int>>(values);
    check(std::ranges::equal(materialized, std::vector{3, 1, 4}), "materialization consumes actual elements rather than hint slots");
    std::cout << "observed vector capacity=" << materialized.capacity() << '\n';
    return verified("reserve_hint");
#else
    return unavailable("reserve_hint", "requires __cpp_lib_ranges_reserve_hint >= 202502L");
#endif
}
