// =============================================================================
// tests/sync_wait_test.cpp —— 单元测试：mini::sync_wait
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第六层验证"
//
// 关键约束验证：返回类型必须是 std::optional<std::tuple<Ts...>>，
//                core task 成功有值，error 通道 rethrow。
// =============================================================================

#include "mini/sync_wait.hpp"
#include "mini/task.hpp"

#include "coroutine_study/exercise_check.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

static mini::task<int> value() {
    co_return 42;
}

static mini::task<void> value_void(bool& reached) {
    reached = true;
    co_return;
}

static mini::task<int> fail() {
    throw std::runtime_error{"boom"};
    co_return 0;
}

static void run() {
    std::cout << "[test] mini::sync_wait basic\n";

    static_assert(std::is_same_v<decltype(mini::sync_wait(value())),
                                 std::optional<std::tuple<int>>>);

    auto opt = mini::sync_wait(value());
    coroutine_study::check(opt.has_value(), "sync_wait did not return a value");
    auto [v] = *opt;
    coroutine_study::check(v == 42, "sync_wait value mismatch");

    bool reached = false;
    auto void_opt = mini::sync_wait(value_void(reached));
    coroutine_study::check(void_opt.has_value(), "sync_wait void returned stopped");
    coroutine_study::check(reached, "sync_wait void task did not run");

    bool caught = false;
    try {
        (void)mini::sync_wait(fail());
    } catch (const std::runtime_error&) {
        caught = true;
    }
    coroutine_study::check(caught, "sync_wait did not rethrow task error");

    std::cout << "  ok: value, void, error\n";
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
