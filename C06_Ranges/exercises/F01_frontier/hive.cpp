#include <frontier_status.hpp>
#if __has_include(<hive>)
#include <hive>
#include <ranges>
#endif

int main() {
#if __has_include(<hive>) && defined(__cpp_lib_hive) && __cpp_lib_hive >= 202502L
    std::hive<int> values;
    auto keep = values.insert(10);
    auto* address = &*keep;
    auto removed = values.insert(20);
    values.erase(removed);
    for (int i = 0; i != 256; ++i) values.insert(i);
    check(&*keep == address && *keep == 10, "insertion and other-element erase preserve retained element");
    static_assert(std::ranges::bidirectional_range<decltype(values)>);
    static_assert(!std::ranges::random_access_range<decltype(values)>);
    int sum = 0;
    for (int value : values) sum += value;
    check(values.size() == 257 && sum == 32650, "iteration skips erased slots and reaches all live elements");
    values.clear();
    check(values.empty(), "clear removes all live elements");
    return verified("hive");
#else
    return unavailable("hive", "requires <hive> and macro >= 202502L");
#endif
}
