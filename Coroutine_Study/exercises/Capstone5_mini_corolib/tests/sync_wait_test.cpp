// =============================================================================
// tests/sync_wait_test.cpp —— 单元测试：mini::sync_wait
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第六层验证"
//
// 关键约束验证：返回类型必须是 std::optional<std::tuple<Ts...>>，
//                stopped 通道返回空 optional，error 通道 rethrow。
// =============================================================================

#include "mini/sync_wait.hpp"
#include "mini/task.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <tuple>

int main() {
    std::cout << "[test] mini::sync_wait basic\n";

    // TODO[必做]: 启用后：
    //   auto opt = mini::sync_wait([]() -> mini::task<int> {
    //       co_return 42;
    //   }());
    //   assert(opt);
    //   auto [v] = *opt;
    //   assert(v == 42);

    std::cout << "  (skip: sync_wait not yet implemented)\n";
    return 0;
}
