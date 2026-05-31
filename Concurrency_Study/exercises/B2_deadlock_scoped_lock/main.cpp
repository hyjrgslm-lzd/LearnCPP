// =====================================================================
// 练习 B2_deadlock_scoped_lock：死锁与 scoped_lock 多锁原子获取
//   对应文档：Concurrency_Study/03-模块B-互斥与锁.md 的「练习 B-2」
//
//   官方参考：
//     std::scoped_lock  https://en.cppreference.com/w/cpp/thread/scoped_lock
//     std::lock         https://en.cppreference.com/w/cpp/thread/lock
//     std::adopt_lock   https://en.cppreference.com/w/cpp/thread/lock_tag
//
//   学习目标：
//     1. 用两把 mutex 以「相反顺序」加锁，亲手制造一个真实死锁。
//     2. 用 std::scoped_lock 一次性「原子获取」多锁，破坏循环等待，消除死锁。
//     3. 默写死锁四个必要条件，理解修复原理不是「加超时」而是「破坏循环等待」。
//     4. (进阶) std::lock + adopt_lock 写法 / 固定加锁顺序。
//
//   死锁四条件(Coffman)：互斥 / 持有并等待 / 不可抢占 / 循环等待。
//   破坏任意一个即可避免；scoped_lock 破坏「持有并等待 + 循环等待」。
//
//   ⚠️ 死锁演示版默认关闭(kRunDeadlockDemo=false)，否则会永久挂起、卡住
//      整个测试驱动。手动改成 true 单独观察后请改回 false。
//
//   本文件用 stdout 测试驱动；未完成 TODO 用最小占位保证 MSVC 可编译可运行。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

namespace {

// ⚠️ 开关：true 时运行「会真正死锁」的反例版本（程序将永久挂起）。
//    默认 false：跳过死锁版，只运行 scoped_lock 修复版，保证测试能跑完。
constexpr bool kRunDeadlockDemo = false;

// ---------------------------------------------------------------------
// 反例（必做 1）：两把锁以相反顺序加锁 —— 会死锁。
//   线程1：先 mA 后 mB；线程2：先 mB 后 mA。
//   中间的 sleep 制造时间窗，使死锁几乎必然复现。
//   如何观察死锁：程序不退出，日志停在「已锁第一把、正在等第二把」后再无输出。
// ---------------------------------------------------------------------
void deadlock_demo() {
    cs::println("\n=== 反例：相反顺序加锁会死锁（必做1，仅在开关打开时运行） ===");
    std::mutex mA, mB;

    std::thread t1([&] {
        std::lock_guard<std::mutex> la(mA);
        cs::logf("t1 已锁 mA，正在请求 mB ...");
        std::this_thread::sleep_for(50ms); // 制造时间窗
        std::lock_guard<std::mutex> lb(mB); // 这里会卡死(t2 持有 mB)
        cs::logf("t1 同时持有 mA+mB (若打印说明这次侥幸没死锁)");
    });
    std::thread t2([&] {
        std::lock_guard<std::mutex> lb(mB);
        cs::logf("t2 已锁 mB，正在请求 mA ...");
        std::this_thread::sleep_for(50ms);
        std::lock_guard<std::mutex> la(mA); // 这里会卡死(t1 持有 mA)
        cs::logf("t2 同时持有 mB+mA (若打印说明这次侥幸没死锁)");
    });

    t1.join(); // 死锁时永远 join 不回来
    t2.join();
    cs::logf("deadlock_demo 结束(只有没死锁时才会到这)");
}

// ---------------------------------------------------------------------
// 修复版（必做 2）：std::scoped_lock 一次性原子获取两把锁。
//   两个线程即使书写顺序相反，scoped_lock 内部用统一的死锁避免算法获取，
//   要么全拿到、要么一把都不持有 —— 破坏循环等待，永不死锁。
// ---------------------------------------------------------------------
void scoped_lock_fix() {
    cs::println("\n=== 修复：scoped_lock 原子获取多锁（必做2） ===");
    std::mutex mA, mB;
    int balanceA = 100, balanceB = 100;

    auto transfer = [&](const char* who, std::mutex& first, std::mutex& second,
                        int& from, int& to, int amount) {
        // TODO [必做 2]: 用一行 std::scoped_lock 同时锁住 first 与 second，
        //   替换掉下面的「占位：分散单锁」实现。CTAD 可省略模板参数：
        //     std::scoped_lock lk(first, second);
        //     from -= amount; to += amount;
        //     cs::logf(who, " 转账完成");
        //   关键：无论 first/second 实参顺序如何，scoped_lock 都不会死锁。
        //
        // 最小占位（用 std::lock 原子获取两把，等价且安全，保证可运行不死锁；
        //   填好上面的 scoped_lock 后请用那一行替换本占位）：
        std::lock(first, second);
        std::lock_guard<std::mutex> l1(first, std::adopt_lock);
        std::lock_guard<std::mutex> l2(second, std::adopt_lock);
        from -= amount;
        to += amount;
        cs::logf(who, " [占位:std::lock 原子获取] 转账完成");
    };

    // 两个线程参数顺序相反：填好 scoped_lock 后也绝不死锁。
    std::thread t1([&] { transfer("t1", mA, mB, balanceA, balanceB, 10); });
    std::thread t2([&] { transfer("t2", mB, mA, balanceB, balanceA, 5); });
    t1.join();
    t2.join();
    cs::logf("balanceA=", balanceA, " balanceB=", balanceB,
             " (总额应守恒=200)");
}

// ---------------------------------------------------------------------
// 进阶 1：std::lock + adopt_lock —— scoped_lock 的「手动语法糖展开」。
// ---------------------------------------------------------------------
void std_lock_adopt() {
    cs::println("\n=== 进阶1：std::lock + adopt_lock（scoped_lock 的底层等价） ===");
    std::mutex mA, mB;

    auto worker = [&](const char* who) {
        // TODO [进阶 1]: 用 std::lock 原子锁两把，再用 adopt_lock 接管所有权。
        //   参考实现：
        //     std::lock(mA, mB);
        //     std::lock_guard<std::mutex> l1(mA, std::adopt_lock);
        //     std::lock_guard<std::mutex> l2(mB, std::adopt_lock);
        //     cs::logf(who, " 通过 std::lock 原子获取两把锁");
        //   注意：adopt_lock 表示「锁已被我持有，你只负责析构时解锁」，
        //         不可写成 lock_guard l1(mA)（会再次上锁 -> UB）。
        //
        // 最小占位（等价正确实现，已可运行）：
        std::lock(mA, mB);
        std::lock_guard<std::mutex> l1(mA, std::adopt_lock);
        std::lock_guard<std::mutex> l2(mB, std::adopt_lock);
        cs::logf(who, " 通过 std::lock 原子获取两把锁");
    };

    std::thread t1([&] { worker("t1"); });
    std::thread t2([&] { worker("t2"); });
    t1.join();
    t2.join();
}

// ---------------------------------------------------------------------
// 进阶 2：固定加锁顺序 —— 永远先锁地址较小的那把，同样避免死锁。
// ---------------------------------------------------------------------
void fixed_order() {
    cs::println("\n=== 进阶2：固定加锁顺序(按地址)避免死锁 ===");
    std::mutex mA, mB;

    auto worker = [&](const char* who) {
        // TODO [进阶 2]: 永远先锁地址较小的 mutex，破坏循环等待。
        //   参考实现：
        //     std::mutex* first  = (&mA < &mB) ? &mA : &mB;
        //     std::mutex* second = (&mA < &mB) ? &mB : &mA;
        //     std::lock_guard<std::mutex> l1(*first);
        //     std::lock_guard<std::mutex> l2(*second);
        //     cs::logf(who, " 按固定顺序获取两把锁");
        //
        // 最小占位（等价正确实现）：
        std::mutex* first = (&mA < &mB) ? &mA : &mB;
        std::mutex* second = (&mA < &mB) ? &mB : &mA;
        std::lock_guard<std::mutex> l1(*first);
        std::lock_guard<std::mutex> l2(*second);
        cs::logf(who, " 按固定顺序获取两把锁");
    };

    std::thread t1([&] { worker("t1"); });
    std::thread t2([&] { worker("t2"); });
    t1.join();
    t2.join();
}

} // namespace

int main() {
    cs::println("==== B2_deadlock_scoped_lock: 死锁与 scoped_lock 多锁 ====");

    if (kRunDeadlockDemo) {
        // 仅在你想亲眼观察死锁时把 kRunDeadlockDemo 改为 true。
        // 注意：程序会在此永久挂起，需手动结束进程。观察后请改回 false。
        deadlock_demo();
    } else {
        cs::println("\n(死锁反例已关闭。把 kRunDeadlockDemo 改为 true 可亲眼观察永久挂起。)");
    }

    scoped_lock_fix(); // 必做2：scoped_lock 修复
    std_lock_adopt();  // 进阶1：std::lock + adopt_lock
    fixed_order();     // 进阶2：固定加锁顺序

    cs::println("\n==== 跑完。修复原理：原子获取多锁 -> 破坏循环等待(非超时重试)。 ====");
    return 0;
}
