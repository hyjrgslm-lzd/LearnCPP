// =====================================================================
// 练习 B3_call_once：call_once 一次性初始化
//   对应文档：Concurrency_Study/03-模块B-互斥与锁.md 的「练习 B-3」
//
//   官方参考：
//     std::call_once   https://en.cppreference.com/w/cpp/thread/call_once
//     std::once_flag   https://en.cppreference.com/w/cpp/thread/once_flag
//     局部静态线程安全初始化(magic statics)：
//       https://en.cppreference.com/w/cpp/language/storage_duration
//
//   学习目标：
//     1. 用 std::call_once + std::once_flag 做线程安全惰性初始化。
//     2. N 个线程并发触发，验证初始化「恰好只执行一次」。
//     3. 与「函数内 static 局部变量(magic statics, C++11 线程安全)」对比取舍。
//     4. (进阶) call_once 初始化抛异常时不翻转 once_flag 的语义；
//        以及为什么不该手写 double-checked locking。
//
//   本文件用 stdout 测试驱动；未完成 TODO 用最小占位保证 MSVC 可编译可运行。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 8;

// 用一个原子计数器记录「初始化体被真正执行了几次」，最终应为 1。
std::atomic<int> g_init_count{0};

// ---------------------------------------------------------------------
// 必做 1 + 2：call_once 惰性初始化，并发触发验证只执行一次。
// ---------------------------------------------------------------------
struct Widget {
    int value = 0;
};

std::once_flag g_flag;
std::unique_ptr<Widget> g_widget;

Widget& get_resource() {
    // TODO [必做 1]: 用 std::call_once(g_flag, ...) 做惰性初始化。
    //   保证：对同一个 once_flag，传入的可调用对象在所有线程中总共只成功执行一次。
    //   参考实现：
    //     std::call_once(g_flag, [] {
    //         cs::log("正在初始化 Widget ...（应只出现一次）");
    //         g_init_count.fetch_add(1, std::memory_order_relaxed);
    //         g_widget = std::make_unique<Widget>(Widget{123});
    //     });
    //     return *g_widget;
    //
    // 最小占位（等价正确实现，已可运行）：
    std::call_once(g_flag, [] {
        cs::log("正在初始化 Widget ...（应只出现一次）");
        g_init_count.fetch_add(1, std::memory_order_relaxed);
        g_widget = std::make_unique<Widget>(Widget{123});
    });
    return *g_widget;
}

void scenario_call_once() {
    cs::println("\n=== 场景1：call_once 并发触发，验证只初始化一次（必做1+2） ===");
    std::atomic<bool> go{false};
    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&go, i] {
            while (!go.load(std::memory_order_acquire)) { /* 起跑栅栏自旋 */ }
            Widget& w = get_resource();
            cs::logf("thread#", i, " 拿到 Widget.value=", w.value);
        });
    }
    go.store(true, std::memory_order_release); // 统一放行，制造并发竞争
    for (auto& t : ts) t.join();
    cs::logf("g_init_count=", g_init_count.load(), "  (必须 == 1)");
}

// ---------------------------------------------------------------------
// 必做 3：magic statics 对照 —— 函数内 static 局部变量(C++11 线程安全)。
// ---------------------------------------------------------------------
std::atomic<int> g_ctor_count{0};

struct MagicWidget {
    MagicWidget() {
        g_ctor_count.fetch_add(1, std::memory_order_relaxed);
        cs::log("MagicWidget 构造 ...（应只出现一次）");
    }
    int value = 456;
};

MagicWidget& get_resource2() {
    // TODO [必做 3]: 用「函数内 static 局部变量」做惰性初始化。
    //   C++11 起标准保证其初始化线程安全：首个线程构造，其余阻塞等待。
    //   参考实现：
    //     static MagicWidget w;   // 线程安全的局部静态初始化(magic statics)
    //     return w;
    //
    // 最小占位（等价正确实现）：
    static MagicWidget w;
    return w;
}

void scenario_magic_statics() {
    cs::println("\n=== 场景2：magic statics 对照，构造也只一次（必做3） ===");
    std::atomic<bool> go{false};
    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&go, i] {
            while (!go.load(std::memory_order_acquire)) {}
            MagicWidget& w = get_resource2();
            cs::logf("thread#", i, " 拿到 MagicWidget.value=", w.value);
        });
    }
    go.store(true, std::memory_order_release);
    for (auto& t : ts) t.join();
    cs::logf("g_ctor_count=", g_ctor_count.load(), "  (必须 == 1)");
}

// ---------------------------------------------------------------------
// 进阶 1：初始化抛异常时 once_flag 不翻转，下次会重试。
// ---------------------------------------------------------------------
void scenario_call_once_throws() {
    cs::println("\n=== 场景3：call_once 初始化抛异常 -> 不翻转，下次重试（进阶1） ===");
    std::once_flag flag;
    std::atomic<int> attempts{0};
    bool fail_first = true;

    auto try_init = [&] {
        // TODO [进阶 1]: 第一次故意抛异常，观察 once_flag 不翻转、第二次重试。
        //   参考实现见下方占位（已是正确实现）：
        try {
            std::call_once(flag, [&] {
                attempts.fetch_add(1, std::memory_order_relaxed);
                if (fail_first) {
                    fail_first = false;
                    throw std::runtime_error("首次初始化故意失败");
                }
                cs::log("初始化成功（第二次尝试）");
            });
        } catch (const std::exception& e) {
            cs::logf("捕获初始化异常: ", e.what(), "（once_flag 未翻转，可重试）");
        }
    };

    try_init(); // 第一次：抛异常
    try_init(); // 第二次：因 flag 未翻转，会再次进入初始化体并成功
    cs::logf("attempts=", attempts.load(), "  (应 == 2：首次失败 + 二次成功)");
}

} // namespace

int main() {
    cs::println("==== B3_call_once: 线程安全一次性初始化 ====");
    scenario_call_once();        // 必做1+2：call_once 只一次
    scenario_magic_statics();    // 必做3：magic statics 对照
    scenario_call_once_throws(); // 进阶1：抛异常重试语义
    cs::println("\n==== 跑完。对照 README / 03-模块B 文档自检验收点。 ====");
    return 0;
}
