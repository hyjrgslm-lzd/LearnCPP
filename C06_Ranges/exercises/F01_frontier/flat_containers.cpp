#include <check.hpp>
#include <algorithm>
#include <flat_map>
#include <flat_set>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

int main() {
    std::flat_map<int, std::string> names;
    names.insert_or_assign(20, "beta");
    names.insert_or_assign(10, "alpha");
    names.insert_or_assign(20, "updated");
    check(names.size() == 2 && names.at(20) == "updated", "flat map updates equivalent key");
    check(names.begin()->first == 10, "flat map iteration follows key order");
    static_assert(std::ranges::random_access_range<decltype(names)>);
    static_assert(!std::ranges::contiguous_range<decltype(names)>);
    check(std::ranges::is_sorted(names.keys()), "flat map keeps its keys sorted");
    check(names.keys().size() == names.values().size(), "parallel key and value storage stay paired");
    auto storage = std::move(names).extract();
    std::flat_map<int, std::string> restored;
    restored.replace(std::move(storage.keys), std::move(storage.values));
    check(restored.at(10) == "alpha" && restored.at(20) == "updated", "extract and replace preserve associations");
    std::flat_set<int> unique{4, 1, 4, 2};
    check(std::ranges::equal(unique, std::vector{1, 2, 4}), "flat set sorts and deduplicates");
    unique.erase(2);
    check(!unique.contains(2) && unique.contains(4), "flat set erase preserves remaining keys");
    std::cout << "PASS flat containers: order, uniqueness, proxy iteration, paired storage\n";
}
