#include <check.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <queue>
#include <ranges>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

namespace {

void check_array() {
    std::array<int, 3> values{1, 2, 3};
    static_assert(std::ranges::sized_range<decltype(values)>);
    static_assert(std::ranges::contiguous_range<decltype(values)>);
    check(values.size() == 3, "array size is fixed");
    values[1] = 20;
    check(values[1] == 20, "array mutates existing elements");
}

void check_vector_growth_and_span() {
    std::vector<int> values;
    values.reserve(3);
    values.push_back(1);
    auto* stable = values.data();
    std::span<int> borrowed = values;
    values.push_back(2);
    values.push_back(3);
    check(values.data() == stable, "vector push within capacity keeps storage");
    check(borrowed.data() == stable, "span borrows the same storage before reallocation");

    values.push_back(4);
    check(values.data() != stable, "vector growth changes storage in this controlled setup");
    check(values.size() == 4, "vector keeps values after growth");
}

void check_deque() {
    std::deque<int> values{2, 3};
    static_assert(std::ranges::random_access_range<decltype(values)>);
    static_assert(!std::ranges::contiguous_range<decltype(values)>);
    int& middle = values[0];
    values.push_front(1);
    values.push_back(4);
    check(middle == 2, "deque end insertions keep existing element references usable");
    check(values.front() == 1 && values.back() == 4, "deque supports both ends");
}

void check_lists() {
    std::list<std::string> words{"alpha", "gamma"};
    auto first = words.begin();
    const auto* first_address = std::addressof(*first);
    words.insert(std::next(first), "beta");
    check(std::addressof(*first) == first_address, "list insertion keeps other node addresses");
    check(words.size() == 3, "list inserts a node");

    std::forward_list<int> numbers{1, 3};
    auto before_second = numbers.before_begin();
    numbers.insert_after(before_second, 0);
    auto after_first = numbers.begin();
    numbers.erase_after(after_first);
    check(std::ranges::equal(numbers, std::array{0, 3}), "forward_list modifies after a position");
}

void check_views_and_adaptors() {
    std::string text = "abc";
    std::string_view view = text;
    check(view.size() == 3 && view[0] == 'a', "string_view borrows characters");

    std::stack<int> stack;
    stack.push(1);
    stack.push(2);
    check(stack.top() == 2, "stack exposes last-in first-out top");
    stack.pop();
    check(stack.top() == 1, "stack pop reveals previous top");

    std::queue<int> queue;
    queue.push(1);
    queue.push(2);
    check(queue.front() == 1 && queue.back() == 2, "queue exposes front and back");
    queue.pop();
    check(queue.front() == 2, "queue pop removes the oldest element");
}

} // namespace

int main() {
    check_array();
    check_vector_growth_and_span();
    check_deque();
    check_lists();
    check_views_and_adaptors();
    std::cout << "L02_sequence_containers observation OK\n";
}
