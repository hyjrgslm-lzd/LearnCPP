// =============================================================================
// tests/task_test.cpp —— 单元测试：mini::task<T>
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第三层验证"
//
// 用法：每个组件一个独立 test 文件，便于改一个组件只重编译一个 TU。
// =============================================================================

#include "mini/task.hpp"

#include <cassert>
#include <iostream>

static mini::task<int> compute() {
    co_return 42;
}

int main() {
    std::cout << "[test] mini::task<T> basic\n";

    auto t = compute();
    // TODO[必做]: 等 sync_wait 实现后改为：
    //   auto opt = mini::sync_wait(std::move(t));
    //   assert(opt && std::get<0>(*opt) == 42);
    //
    // 当前用最朴素的方式跑：直接 resume，再读 promise.result_。
    t.h_.resume();
    auto& r = t.h_.promise().result_;
    assert(r.index() == 1);
    assert(std::get<1>(r) == 42);
    std::cout << "  ok: result = 42\n";
    return 0;
}
