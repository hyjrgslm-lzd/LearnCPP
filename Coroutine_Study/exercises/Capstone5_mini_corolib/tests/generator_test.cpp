// =============================================================================
// tests/generator_test.cpp —— 单元测试：mini::generator<T>
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第四层验证"
// =============================================================================

#include "mini/generator.hpp"

#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

static mini::generator<int> seq(int n) {
    for (int i = 0; i < n; ++i) co_yield i;
}

int main() {
    std::cout << "[test] mini::generator<T> basic\n";

    int sum = 0;
    for (int x : seq(5)) sum += x;
    assert(sum == 0 + 1 + 2 + 3 + 4);
    std::cout << "  ok: sum(0..4) = " << sum << "\n";

    // J-1 陷阱 8 验证：yield 临时量是否安全（promise 必须按值存）
    auto g = []() -> mini::generator<int> {
        co_yield std::max(1, 2);    // 临时量 prvalue
        co_yield std::max(3, 4);
    }();
    std::vector<int> vs;
    for (int v : g) vs.push_back(v);
    assert(vs == (std::vector<int>{2, 4}));
    std::cout << "  ok: yield prvalue (J-1 trap 8 immune)\n";

    return 0;
}
