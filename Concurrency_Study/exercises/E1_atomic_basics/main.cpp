// =====================================================================
// 练习 E-1：原子操作基础（std::atomic / atomic_flag）
//   对应文档：Concurrency_Study/07-模块E-原子操作基础.md 的 练习 E-1
//
//   学习目标：
//     - 掌握 std::atomic<T>（原子类型，<atomic>，C++11）的核心操作：
//       load / store / exchange，以及读-改-写（RMW）家族 fetch_add/sub/and/or/xor；
//     - 亲眼看到“非原子 ++ 在多线程下丢更新（lost update）”，再用
//       atomic<int> 的 fetch_add 修复，理解“原子 = 不可分割的整体动作”；
//     - 区分 is_lock_free()（成员，运行期）与 is_always_lock_free
//       （C++17 静态 constexpr 常量，编译期）；
//     - 用 std::atomic_flag（C++11；test_and_set / clear；C++20 起加 test()）
//       搭一个最朴素的自旋锁（spinlock）雏形。
//
//   关于内存序（memory order）：本模块所有原子操作都用“默认内存序”——
//   即顺序一致（sequential consistency，seq_cst），它是最强、最易推理的
//   保证。为什么很多场景能用更弱的序（acquire/release/relaxed）换性能，
//   留到模块 F 专门展开；本模块只关心“原子操作本身做了什么”。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic
//     - https://en.cppreference.com/w/cpp/atomic/atomic/fetch_add
//     - https://en.cppreference.com/w/cpp/atomic/atomic/is_lock_free
//     - https://en.cppreference.com/w/cpp/atomic/atomic/is_always_lock_free
//     - https://en.cppreference.com/w/cpp/atomic/atomic_flag
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.2
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target E1_atomic_basics --config Release
//     ./build-vs2026/E1_atomic_basics/Release/E1_atomic_basics.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <thread>
#include <vector>

// =====================================================================
// is_always_lock_free 是 C++17 起的 static constexpr 成员常量：
// 它在“编译期”就告诉你该 atomic 特化在“任何该平台的运行实例”上都是
// 无锁（lock-free）的。因为它是编译期常量，所以可以直接 static_assert。
// 在主流 64 位平台上，atomic<int> / atomic<bool> 几乎必为 true。
// =====================================================================
static_assert(std::atomic<int>::is_always_lock_free,
              "本平台 atomic<int> 预期为无锁（lock-free）");

// =====================================================================
// 必做 1：原子自增 vs 非原子自增——亲眼看到“丢更新”。
//   多个线程对同一个计数器各自做 N 次自增。
//   - 非原子版：普通 int 的 ++ 是“读 → 加 1 → 写回”三步，线程间会
//     互相覆盖中间结果（数据竞争 data race，本身就是未定义行为），
//     最终结果几乎总是 < 期望值。
//   - 原子版：atomic<int>::fetch_add 把“读-改-写”合成一个不可分割的
//     动作，任何时刻只有一个线程能完成它，结果必然精确等于期望值。
// =====================================================================
void demo_atomic_increment() {
    cs::println("============ 必做 1：原子自增 vs 非原子自增 ============");

    constexpr int kThreads = 8;
    constexpr int kPerThread = 100000;
    constexpr long long kExpected =
        static_cast<long long>(kThreads) * kPerThread;

    // ---- 反面：非原子计数器，演示丢更新 ----
    // 注意：普通 int 被多线程无同步地写是数据竞争（UB），这里仅作教学演示，
    // 真实代码绝不能这么写。我们关心的是“结果通常对不上”。
    int plain = 0;
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < kThreads; ++i) {
            ts.emplace_back([&plain] {
                for (int k = 0; k < kPerThread; ++k) {
                    ++plain; // 读-改-写三步非原子，会被并发覆盖
                }
            });
        }
        for (auto& t : ts) t.join();
    }
    cs::logf("[plain ] 非原子结果 = ", plain, "，期望 = ", kExpected,
             "（", (plain == kExpected ? "侥幸相等" : "丢更新，对不上"), "）");

    // ---- 正面：atomic<int> + fetch_add ----
    std::atomic<int> counter{0};
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < kThreads; ++i) {
            ts.emplace_back([&counter] {
                for (int k = 0; k < kPerThread; ++k) {
                    // TODO [必做 1]: 用原子的读-改-写把计数器加 1。
                    //   要点：fetch_add 是不可分割的整体动作，返回“加之前”的旧值；
                    //   默认内存序是 seq_cst。等价的便捷写法是 counter.fetch_add(1)
                    //   或运算符重载 ++counter / counter += 1（同样是原子的）。
                    //   下面已是正确写法，留作必做 1 的参考实现：
                    counter.fetch_add(1);
                }
            });
        }
        for (auto& t : ts) t.join();
    }
    // atomic<int> 没有隐式转换；用 .load() 取当前值。
    cs::logf("[atomic] 原子结果   = ", counter.load(), "，期望 = ", kExpected,
             "（", (counter.load() == kExpected ? "精确相等" : "异常"), "）");
    cs::println("");
}

// =====================================================================
// 必做 2：load / store / exchange / 其余 RMW + is_lock_free。
//   - load()  ：原子地读出当前值。
//   - store() ：原子地写入新值（不关心旧值）。
//   - exchange()：原子地“写入新值并返回旧值”——读和写一步完成。
//   - fetch_or / fetch_and / fetch_xor：对整型做原子位运算，返回旧值。
//   - is_lock_free()：运行期查询“此实例是否无锁”（与编译期的
//     is_always_lock_free 区分：后者是‘任意实例都无锁’的更强承诺）。
// =====================================================================
void demo_atomic_ops() {
    cs::println("====== 必做 2：load/store/exchange/RMW + is_lock_free ======");

    std::atomic<int> a{10};

    cs::logf("[ops] 初值 load() = ", a.load());

    a.store(42); // 原子写入，丢弃旧值
    cs::logf("[ops] store(42) 后 load() = ", a.load());

    // TODO [必做 2]: 用 exchange 原子地“换入新值并取回旧值”。
    //   要点：exchange 一步完成“读旧 + 写新”，常用于实现锁、状态机的
    //   原子翻转。下面已是正确写法，留作必做 2 的参考实现：
    int old = a.exchange(7);
    cs::logf("[ops] exchange(7) 返回旧值 = ", old, "，现 load() = ", a.load());

    // 整型原子的位运算 RMW：fetch_or / fetch_and / fetch_xor，均返回旧值。
    std::atomic<unsigned> flags{0b0001u};
    unsigned prev = flags.fetch_or(0b0100u); // 置位 bit2
    cs::logf("[ops] fetch_or(0b0100) 旧=", prev, " 新=", flags.load());
    flags.fetch_and(0b0110u); // 保留 bit1、bit2
    cs::logf("[ops] fetch_and(0b0110) 后 = ", flags.load());
    flags.fetch_xor(0b0010u); // 翻转 bit1
    cs::logf("[ops] fetch_xor(0b0010) 后 = ", flags.load());

    // 运行期查询：本实例是否无锁。在主流平台上对 int 通常为 true，
    // 与编译期常量 is_always_lock_free 互相印证。
    cs::logf("[ops] a.is_lock_free() = ", a.is_lock_free(),
             "，atomic<int>::is_always_lock_free = ",
             std::atomic<int>::is_always_lock_free);
    cs::println("");
}

// =====================================================================
// 必做 3：用 atomic_flag 做自旋锁（spinlock）雏形。
//   std::atomic_flag 是 C++ 保证“永远无锁”的最简原子布尔旗标。
//   - test_and_set()：原子地“把旗标置为 set，并返回它之前是否已被 set”。
//   - clear()        ：原子地清回未 set。
//   - test()（C++20）：只读地看当前是否 set，不修改。
//
//   自旋锁逻辑：lock 时反复 test_and_set 直到拿到“之前是未 set”的那一刻
//   （说明锁刚被我抢到）；unlock 时 clear。自旋（spin）= 拿不到就忙等。
//   注意：这是教学雏形，真实场景应优先用 std::mutex（模块 B）；自旋锁
//   只在临界区极短、且不愿让出 CPU 时才划算。
// =====================================================================
class SpinLock {
public:
    void lock() {
        // TODO [必做 3]: 用 atomic_flag 实现自旋获取锁。
        //   要点：test_and_set() 返回“调用前”旗标是否已 set。
        //   - 返回 true  → 之前已被别人 set，说明锁被占着，继续自旋；
        //   - 返回 false → 之前是清的，这一刻是我把它 set 上去的，抢锁成功。
        //   下面已是正确写法，留作必做 3 的参考实现：
        while (flag_.test_and_set()) {
            // 忙等（busy-wait）。C++20 可加 flag_.test() 先只读探测以减少
            //   对缓存行的写争用；本雏形从简，直接自旋。
        }
    }

    void unlock() {
        flag_.clear(); // 原子清旗标，放行下一个等待者
    }

private:
    // atomic_flag 必须用 ATOMIC_FLAG_INIT 或（C++20 起）默认初始化为清状态。
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

void demo_spinlock() {
    cs::println("=========== 必做 3：atomic_flag 自旋锁雏形 ===========");

    SpinLock sl;
    long long guarded = 0; // 受自旋锁保护的普通变量
    constexpr int kThreads = 8;
    constexpr int kPerThread = 50000;

    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&] {
            for (int k = 0; k < kPerThread; ++k) {
                sl.lock();
                ++guarded; // 临界区：同一时刻只有一个线程在此
                sl.unlock();
            }
        });
    }
    for (auto& t : ts) t.join();

    const long long expected =
        static_cast<long long>(kThreads) * kPerThread;
    cs::logf("[spin] 自旋锁保护下结果 = ", guarded, "，期望 = ", expected,
             "（", (guarded == expected ? "精确相等" : "异常"), "）");
    cs::println("");
}

int main() {
    cs::println("==== E1_atomic_basics：atomic 基础 / RMW / atomic_flag ====\n");

    demo_atomic_increment(); // 必做 1：原子自增 vs 丢更新
    demo_atomic_ops();       // 必做 2：load/store/exchange/RMW/is_lock_free
    demo_spinlock();         // 必做 3：atomic_flag 自旋锁雏形

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
