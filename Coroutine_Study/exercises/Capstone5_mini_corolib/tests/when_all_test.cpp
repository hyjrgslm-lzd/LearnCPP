// =============================================================================
// tests/when_all_test.cpp —— 单元测试：mini::when_all
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第五层验证"
// =============================================================================

#include "mini/sync_wait.hpp"
#include "mini/task.hpp"
#include "mini/when_all.hpp"

#include <cassert>
#include <iostream>

static mini::task<int> sum() { co_return 2 + 3; }
static mini::task<int> prod() { co_return 2 * 3; }

int main() {
    std::cout << "[test] mini::when_all basic\n";

    // TODO[必做]: 当 when_all 与 sync_wait 实现完后启用：
    //   auto opt = mini::sync_wait(mini::when_all(sum(), prod()));
    //   assert(opt);
    //   auto [s, p] = std::get<0>(*opt);
    //   assert(s == 5 && p == 6);

    std::cout << "  (skip: when_all not yet implemented)\n";
    return 0;
}
