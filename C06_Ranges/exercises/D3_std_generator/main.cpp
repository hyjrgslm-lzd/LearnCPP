#include <check.hpp>
#include <generator>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

std::generator<int> fib() {
    int a = 0;
    int b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

struct Node {
    int value;
    std::vector<Node> children;
};

std::generator<int> preorder(const Node& node) {
    co_yield node.value;
    for (const Node& child : node.children) {
        co_yield std::ranges::elements_of(preorder(child));
    }
}

std::generator<const int&> flatten(std::vector<std::vector<int>> rows) {
    for (const auto& row : rows) {
        co_yield std::ranges::elements_of(row);
    }
}

int main() {
    static_assert(std::ranges::input_range<std::generator<int>>);
    static_assert(std::ranges::view<std::generator<int>>);
    static_assert(!std::ranges::forward_range<std::generator<int>>);
    static_assert(!std::copyable<std::generator<int>>);
    static_assert(std::movable<std::generator<int>>);

    auto first_ten = fib() | std::views::take(10) | std::ranges::to<std::vector<int>>();
    check((first_ten == std::vector<int>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34}), "generator starts on begin and advances on ++");

    auto g = fib();
    auto it = g.begin();
    check(*it == 0, "begin starts the coroutine to the first yield");
    ++it;
    check(*it == 1, "increment resumes to the next yield");

    Node tree{1, {{2, {{4, {}}}}, {3, {{5, {}}, {6, {}}}}}};
    auto walked = preorder(tree) | std::ranges::to<std::vector<int>>();
    check((walked == std::vector<int>{1, 2, 4, 3, 5, 6}), "elements_of delegates nested generator output");

    auto filtered = preorder(tree)
        | std::views::filter([](int value) { return value >= 3; })
        | std::ranges::to<std::vector<int>>();
    check((filtered == std::vector<int>{4, 3, 5, 6}), "generator composes with ranges adaptors as an input range");

    auto flat = flatten({{1, 2, 3}, {4, 5}, {6}}) | std::ranges::to<std::vector<int>>();
    check((flat == std::vector<int>{1, 2, 3, 4, 5, 6}), "elements_of can flatten ordinary ranges");

    std::cout << "D3 std::generator checks passed\n";
}
