// =============================================================================
// tests/scope_test.cpp —— 单元测试：mini::async_scope + stop_token
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第八层验证"
// =============================================================================

#include "mini/async_scope.hpp"
#include "mini/single_thread_executor.hpp"
#include "mini/stop_token.hpp"
#include "mini/task.hpp"

#include <cassert>
#include <iostream>

int main() {
    std::cout << "[test] mini::async_scope basic\n";

    mini::async_scope          scope;
    mini::in_place_stop_source stop_src;

    // 起点状态：scope 为空
    assert(scope.in_flight() == 0);

    // TODO[必做]: spawn 几个 task<void>，验证 in_flight 计数与 wait_empty()
    //   for (int i = 0; i < 5; ++i) {
    //       scope.spawn([]() -> mini::task<void> { co_return; }());
    //   }
    //   scope.wait_empty();
    //   assert(scope.in_flight() == 0);

    (void)stop_src;
    std::cout << "  ok: empty scope\n";
    return 0;
}
