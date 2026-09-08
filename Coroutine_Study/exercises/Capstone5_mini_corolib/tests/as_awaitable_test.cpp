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

#include "coroutine_study/exercise_check.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <tuple>

#include <stdexec/execution.hpp>
namespace ex = stdexec;

static mini::task<int> bridge_values() {
    int x = co_await mini::as_awaitable(ex::just(42));
    int y = co_await mini::as_awaitable(ex::just(10));
    co_return x + y;
}

static mini::task<int> bridge_error() {
    co_return co_await mini::as_awaitable(
        ex::just_error(std::make_exception_ptr(std::runtime_error{"boom"})));
}

static mini::task<int> bridge_stopped() {
    co_return co_await mini::as_awaitable(ex::just_stopped());
}

static void run() {
    std::cout << "[test] mini::as_awaitable bridge\n";

    auto opt = mini::sync_wait(bridge_values());
    coroutine_study::check(opt.has_value(), "as_awaitable root returned stopped");
    coroutine_study::check(std::get<0>(*opt) == 52, "as_awaitable value mismatch");

    bool error_seen = false;
    try {
        (void)mini::sync_wait(bridge_error());
    } catch (const std::runtime_error&) {
        error_seen = true;
    }
    coroutine_study::check(error_seen, "as_awaitable did not propagate error");

    bool stopped_seen = false;
    try {
        (void)mini::sync_wait(bridge_stopped());
    } catch (const std::runtime_error&) {
        stopped_seen = true;
    }
    coroutine_study::check(stopped_seen, "as_awaitable stopped path did not throw");

    std::cout << "  ok: value, error, stopped\n";
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
