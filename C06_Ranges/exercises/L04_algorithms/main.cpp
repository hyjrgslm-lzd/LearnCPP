#include <check.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

namespace {

enum class Level { info, warn, error };

struct LogRecord {
    int timestamp;
    Level level;
    std::string user_id;
    int bytes;
};

std::vector<LogRecord> sample() {
    return {
        {3, Level::error, "alice", 40},
        {1, Level::info, "bob", 10},
        {2, Level::warn, "alice", 30},
        {4, Level::error, "carol", 20},
        {5, Level::warn, "bob", 50},
    };
}

void check_find_transform_projection() {
    auto records = sample();
    auto error = std::ranges::find(records, Level::error, &LogRecord::level);
    check(error != records.end() && error->user_id == "alice", "find uses projection before equality");

    std::vector<int> bytes;
    std::ranges::transform(records, std::back_inserter(bytes), &LogRecord::bytes);
    check((bytes == std::vector<int>{40, 10, 30, 20, 50}), "transform projection extracts bytes");
}

void check_partition_and_sort() {
    auto records = sample();
    auto middle = std::ranges::stable_partition(records, [](Level l) { return l == Level::error; }, &LogRecord::level);
    check(std::ranges::all_of(std::ranges::subrange(records.begin(), middle.begin()),
              [](const LogRecord& r) { return r.level == Level::error; }),
          "stable_partition moves matching records before returned boundary");
    check(records[0].timestamp == 3 && records[1].timestamp == 4, "stable partition keeps relative order inside true group");

    std::ranges::stable_sort(records, std::less{}, &LogRecord::user_id);
    auto alice = std::ranges::equal_range(records, std::string{"alice"}, std::less{}, &LogRecord::user_id);
    check(std::ranges::distance(alice) == 2, "equal_range requires sorted input with the same projection");
    check(alice.begin()->timestamp == 3, "stable_sort keeps equivalent user order");
}

void check_merge_set_heap() {
    std::vector<int> left{1, 3, 5, 7};
    std::vector<int> right{2, 3, 6};
    std::vector<int> merged;
    std::ranges::merge(left, right, std::back_inserter(merged));
    check((merged == std::vector<int>{1, 2, 3, 3, 5, 6, 7}), "merge preserves sorted order from sorted inputs");

    std::vector<int> intersection;
    std::ranges::set_intersection(left, right, std::back_inserter(intersection));
    check((intersection == std::vector<int>{3}), "set_intersection consumes sorted sets");

    std::vector<int> heap{4, 1, 7, 3, 2};
    std::ranges::make_heap(heap);
    check(heap.front() == 7, "heap front is max");
    check(!std::ranges::is_sorted(heap, std::greater{}), "heap is not a fully sorted range");
    std::ranges::pop_heap(heap);
    int max_value = heap.back();
    heap.pop_back();
    check(max_value == 7, "pop_heap moves max to the old last element");
}

void check_erase_remove_and_fold() {
    auto records = sample();
    auto tail = std::ranges::remove_if(records, [](Level l) { return l == Level::warn; }, &LogRecord::level);
    check(records.size() == 5, "remove_if does not resize the container");
    records.erase(tail.begin(), tail.end());
    check(records.size() == 3, "erase changes vector size after remove_if");

#ifdef __cpp_lib_ranges_fold
    int total = std::ranges::fold_left(records, 0, [](int sum, const LogRecord& r) { return sum + r.bytes; });
#else
    int total = std::accumulate(records.begin(), records.end(), 0,
        [](int sum, const LogRecord& r) { return sum + r.bytes; });
#endif
    check(total == 70, "fold sums the remaining records");
}

} // namespace

int main() {
    check_find_transform_projection();
    check_partition_and_sort();
    check_merge_set_heap();
    check_erase_remove_and_fold();
    std::cout << "L04_algorithms observation OK\n";
}
