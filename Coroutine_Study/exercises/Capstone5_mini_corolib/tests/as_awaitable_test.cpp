// =============================================================================
// tests/as_awaitable_test.cpp —— 单元测试：mini::as_awaitable（stdexec 桥接）
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第九层验证"
//
// 这条测试必须在 stage3 deps 配置好（链 stdexec）的情况下编译。
// =============================================================================

#include "mini/as_awaitable.hpp"
#include "mini/sync_wait.hpp"
#include "mini/task.hpp"

#include <cassert>
#include <iostream>

// #include <stdexec/execution.hpp>
// namespace ex = stdexec;

int main() {
    std::cout << "[test] mini::as_awaitable bridge\n";

    // TODO[必做]: 启用后：
    //   auto bridge_test = []() -> mini::task<int> {
    //       int x = co_await mini::as_awaitable(ex::just(42));
    //       int y = co_await mini::as_awaitable(ex::just(10));
    //       co_return x + y;
    //   };
    //   auto opt = mini::sync_wait(bridge_test());
    //   assert(opt && std::get<0>(*opt) == 52);

    std::cout << "  (skip: as_awaitable not yet implemented)\n";
    return 0;
}
