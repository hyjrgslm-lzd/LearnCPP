// =============================================================================
// tests/when_all_test.cpp —— 单元测试：mini::when_all
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第五层验证"
// =============================================================================

#include "mini/sync_wait.hpp"
#include "mini/task.hpp"
#include "mini/when_all.hpp"

#include "coroutine_study/exercise_check.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <tuple>

static mini::task<int> sum() { co_return 2 + 3; }
static mini::task<int> prod() { co_return 2 * 3; }
static mini::task<int> fail() {
    throw std::runtime_error{"boom"};
    co_return 0;
}

static mini::task<std::tuple<int, int>> both_values() {
    co_return co_await mini::when_all(sum(), prod());
}

static mini::task<std::tuple<int, int>> one_error() {
    co_return co_await mini::when_all(sum(), fail());
}

static void run() {
    std::cout << "[test] mini::when_all basic\n";

    auto opt = mini::sync_wait(both_values());
    coroutine_study::check(opt.has_value(), "when_all root returned stopped");
    auto [s, p] = std::get<0>(*opt);
    coroutine_study::check(s == 5 && p == 6, "when_all value mismatch");

    bool caught = false;
    try {
        (void)mini::sync_wait(one_error());
    } catch (const std::runtime_error&) {
        caught = true;
    }
    coroutine_study::check(caught, "when_all did not propagate an error");

    std::cout << "  ok: values and fail-delay error\n";
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
