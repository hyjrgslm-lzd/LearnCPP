#include <check.hpp>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

namespace {

struct CountedLess {
    int* comparisons{};

    bool operator()(int left, int right) const {
        ++*comparisons;
        return left < right;
    }
};

int linear_find_comparisons(const std::vector<int>& values, int needle) {
    int comparisons = 0;
    for (int value : values) {
        ++comparisons;
        if (value == needle) {
            break;
        }
    }
    return comparisons;
}

int binary_find_comparisons(const std::vector<int>& values, int needle) {
    int comparisons = 0;
    (void)std::lower_bound(values.begin(), values.end(), needle, CountedLess{&comparisons});
    return comparisons;
}

struct GrowthCounts {
    int pushes{};
    int relocations{};
    int final_capacity{};
};

GrowthCounts simulate_doubling_pushes(int count) {
    GrowthCounts result;
    int size = 0;
    int capacity = 0;
    while (size != count) {
        if (size == capacity) {
            result.relocations += size;
            capacity = capacity == 0 ? 1 : capacity * 2;
        }
        ++size;
        ++result.pushes;
    }
    result.final_capacity = capacity;
    return result;
}

int front_insert_moves(int count) {
    int moves = 0;
    std::vector<int> values;
    values.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i != count; ++i) {
        moves += static_cast<int>(values.size());
        values.insert(values.begin(), i);
    }
    return moves;
}

} // namespace

int main() {
    std::vector<int> values(1024);
    std::iota(values.begin(), values.end(), 0);

    check(linear_find_comparisons(values, -1) == 1024, "linear worst case checks every element");
    check(binary_find_comparisons(values, 700) <= 11, "binary search comparisons grow logarithmically");

    const auto growth = simulate_doubling_pushes(1024);
    check(growth.pushes == 1024, "growth simulation records each push");
    check(growth.relocations == 1023, "doubling growth moves old elements less than N times");
    check(growth.final_capacity == 1024, "doubling capacity reaches the requested size");

    check(front_insert_moves(10) == 45, "front insertion moves arithmetic-series elements");

    std::cout << "L01_complexity observation OK\n";
}
