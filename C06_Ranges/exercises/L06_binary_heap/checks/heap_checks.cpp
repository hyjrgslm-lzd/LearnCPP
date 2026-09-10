#include <check.hpp>
#include <finite_max_heap.hpp>

#include <iostream>
#include <optional>
#include <vector>

int main() {
    c06_l06::IntMaxHeap heap(6);
    check(heap.empty() && heap.size() == 0, "new heap is empty");
    check(!heap.peek_max(), "empty heap has no max");
    check(!heap.pop_max(), "empty heap pop returns empty");

    for (int value : {4, 1, 7, 7, 3, 9}) {
        check(heap.push(value), "push accepts values until capacity");
        check(heap.peek_max().has_value(), "nonempty heap has a max");
    }
    check(!heap.push(10), "full heap rejects extra value");
    check(heap.size() == 6, "failed push keeps size");

    std::vector<int> popped;
    while (auto value = heap.pop_max()) {
        popped.push_back(*value);
    }
    check((popped == std::vector<int>{9, 7, 7, 4, 3, 1}), "pop returns descending order");
    check(heap.empty(), "heap is empty after every pop");

    check(heap.push(-1), "heap can be reused after emptying");
    check(heap.pop_max().value() == -1, "single element heap pops its value");
    std::cout << "L06_binary_heap checks OK\n";
}
