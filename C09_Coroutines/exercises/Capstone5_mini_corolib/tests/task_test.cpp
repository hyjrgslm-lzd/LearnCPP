// =============================================================================
// tests/task_test.cpp —— 单元测试：mini::task<T>
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第三层验证"
//
// 用法：每个组件一个独立 test 文件，便于改一个组件只重编译一个 TU。
// =============================================================================

#include "mini/task.hpp"

#include "coroutine_study/exercise_check.hpp"

#include <exception>
#include <iostream>

static mini::task<int> compute() {
    co_return 42;
}

static void run() {
    std::cout << "[test] mini::task<T> basic\n";

    auto t = compute();
    // TODO[必做]: 等 sync_wait 实现后改为：
    //   auto opt = mini::sync_wait(std::move(t));
    //   coroutine_study::check(opt && std::get<0>(*opt) == 42, "task value mismatch");
    //
    // 当前用最朴素的方式跑：直接 resume，再读 promise.result_。
    t.h_.resume();
    auto& r = t.h_.promise().result_;
    coroutine_study::check(r.index() == 1, "task did not store a value");
    coroutine_study::check(std::get<1>(r) == 42, "task value mismatch");
    std::cout << "  ok: result = 42\n";
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
