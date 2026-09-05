#include "coroutine_study/exercise_check.hpp"

#include <generator>
#include <iostream>
#include <vector>

namespace {

int resumed = 0;

std::generator<int> fibonacci(int n) {
    ++resumed;
    int a = 0;
    int b = 1;
    for (int i = 0; i < n; ++i) {
        co_yield a;
        int next = a + b;
        a = b;
        b = next;
    }
}

std::generator<int> fibonacci_inf() {
    int a = 0;
    int b = 1;
    for (;;) {
        co_yield a;
        int next = a + b;
        a = b;
        b = next;
    }
}

} // namespace

int main() {
    using coroutine_study::check;

    auto g = fibonacci(10);
    check(resumed == 0, "std::generator body must be lazy before begin()");

    std::vector<int> got;
    for (int v : g) got.push_back(v);
    check((got == std::vector<int>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34}), "fibonacci(10)");
    check(resumed == 1, "first iteration resumes the generator");

    got.clear();
    int count = 0;
    for (int v : fibonacci_inf()) {
        got.push_back(v);
        if (++count == 10) break;
    }
    check((got == std::vector<int>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34}), "infinite fibonacci prefix");

    std::cout << "A1_reference OK\n";
}
