#include "mini_ref/mini.hpp"

#include <cstdlib>
#include <vector>

static mini_ref::generator<int> fib() {
    int a = 0;
    int b = 1;
    for (int i = 0; i < 10; ++i) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

int main() {
    std::vector<int> values;
    for (int v : fib()) values.push_back(v);
    if (values != std::vector<int>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34}) std::abort();
}
