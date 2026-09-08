// =============================================================================
// tests/scope_test.cpp —— 单元测试：mini::async_scope + stop_token
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第八层验证"
// =============================================================================

#include "mini/async_scope.hpp"
#include "mini/single_thread_executor.hpp"
#include "mini/stop_token.hpp"
#include "mini/task.hpp"

#include "coroutine_study/exercise_check.hpp"

#include <atomic>
#include <exception>
#include <iostream>

static mini::task<void> counted(std::atomic<int>& count) {
    ++count;
    co_return;
}

static void run() {
    std::cout << "[test] mini::async_scope basic\n";

    mini::async_scope          scope;
    mini::in_place_stop_source stop_src;
    std::atomic<int>           count{0};

    // 起点状态：scope 为空
    coroutine_study::check(scope.in_flight() == 0, "new scope is not empty");

    for (int i = 0; i < 4; ++i) {
        scope.spawn(counted(count));
    }
    scope.wait_empty();
    coroutine_study::check(scope.in_flight() == 0, "scope did not drain");
    coroutine_study::check(count == 4, "scope did not run all spawned tasks");

    (void)stop_src;
    std::cout << "  ok: spawned work drained\n";
}

int main() {
    try {
        run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "starter check failed: " << e.what() << "\n";
        return 1;
    }
}
