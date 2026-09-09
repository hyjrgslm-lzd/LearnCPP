// =============================================================================
// src/main.cpp —— mini 协程库 demo driver
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"最终验证"
//
// 本 driver 跑通 14-mini §"最终验证" 中前 4 条：
//   验证 1：task + sync_wait（含 as_awaitable 桥接 stdexec::just）
//   验证 2：generator + ranges
//   验证 3：when_all + 类型安全
//   验证 4：async_scope + stop_token
//
// 验证 5 (HALO) 需要看编译器诊断输出，不是运行时 demo；
// 验证 6 (ASan) 把本 main 用 -fsanitize=address 跑一遍即可。
// =============================================================================

#include "mini/as_awaitable.hpp"
#include "mini/async_scope.hpp"
#include "mini/generator.hpp"
#include "mini/single_thread_executor.hpp"
#include "mini/stop_token.hpp"
#include "mini/sync_wait.hpp"
#include "mini/task.hpp"
#include "mini/when_all.hpp"

#include <cassert>
#include <iostream>
#include <ranges>
#include <vector>

// 真实实现需要（CMakeLists 已链 stdexec）：
// #include <stdexec/execution.hpp>
// namespace ex = stdexec;

// =============================================================================
// 验证 1：task + sync_wait
// =============================================================================
static void demo_task_sync_wait() {
    std::cout << "--- Demo 1: task + sync_wait ---\n";

    // auto compute = []() -> mini::task<int> {
    //     int x = co_await mini::as_awaitable(stdexec::just(21));
    //     co_return x * 2;
    // };
    // auto opt = mini::sync_wait(compute());
    // assert(opt && std::get<0>(*opt) == 42);
    // std::cout << "  result = " << std::get<0>(*opt) << " (expect 42)\n";

    std::cout << "  (TODO: 等 sync_wait/as_awaitable 实现完后取消注释)\n";
}

// =============================================================================
// 验证 2：generator + ranges
// =============================================================================
static mini::generator<int> fib() {
    int a = 0, b = 1;
    co_yield a;
    co_yield b;
    for (int i = 0; i < 8; ++i) {
        int c = a + b;
        co_yield c;
        a = b; b = c;
    }
}

static void demo_generator() {
    std::cout << "--- Demo 2: generator + ranges ---\n";

    std::vector<int> values;
    int taken = 0;
    for (int v : fib()) { if (taken++ >= 10) break; values.push_back(v); }

    std::cout << "  fib10 = ";
    for (int v : values) std::cout << v << ' ';
    std::cout << "\n  (expect 0 1 1 2 3 5 8 13 21 34)\n";
}

// =============================================================================
// 验证 3：when_all + 类型安全
// =============================================================================
static void demo_when_all() {
    std::cout << "--- Demo 3: when_all ---\n";

    // auto [sum, prod] = std::get<0>(*mini::sync_wait(
    //     mini::when_all(
    //         []() -> mini::task<int> { co_return 2 + 3; }(),
    //         []() -> mini::task<int> { co_return 2 * 3; }()
    //     )
    // ));
    // assert(sum == 5 && prod == 6);

    std::cout << "  (TODO: 等 when_all/sync_wait 实现完后取消注释)\n";
}

// =============================================================================
// 验证 4：async_scope + stop_token
// =============================================================================
static void demo_async_scope() {
    std::cout << "--- Demo 4: async_scope + stop_token ---\n";

    mini::single_thread_executor   ex_;
    mini::async_scope              scope;
    mini::in_place_stop_source     stop_src;

    // for (int i = 0; i < 10; ++i) {
    //     scope.spawn(/* on(ex_.get_scheduler(), task<void>) */);
    // }
    // scope.wait_empty();

    (void)ex_; (void)scope; (void)stop_src;
    std::cout << "  in_flight = " << scope.in_flight() << " (expect 0)\n";
}

int main() {
    std::cout << "===== Capstone 5: mini coroutine library demo =====\n";
    demo_task_sync_wait();
    demo_generator();
    demo_when_all();
    demo_async_scope();
    std::cout << "===== Done =====\n";
    return 0;
}
