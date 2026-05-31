// =====================================================================
// 练习 F-1：release-acquire 同步（release / acquire 配对发布非原子数据）
//   对应文档：Concurrency_Study/08-模块F-内存模型与memory_order.md 的 练习 F-1
//
//   学习目标：
//     - 用 std::atomic<bool> 的 release 写 / acquire 读配对，
//       安全发布一块非原子（non-atomic）payload；
//     - 理解 synchronizes-with（同步于）如何由“acquire 读到了 release 写的值”
//       建立，进而构成 happens-before（先行于），让发布方在 release 写之前
//       （sequenced-before）的所有写对消费方在 acquire 读之后可见；
//     - 体会为什么把 acquire 换成 relaxed 会破坏可见性保证：
//       relaxed 不与那次 release 写建立 synchronizes-with，
//       于是读 payload 构成数据竞争（data race）→ 未定义行为（UB）。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/memory_order
//       （release-acquire ordering 一节，及其 message-passing 示例）
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3
//     - Herb Sutter, "atomic<> Weapons: The C++ Memory Model and Modern
//       Hardware"（CppCon/C++ and Beyond 演讲）
//     - Mara Bos, 《Rust Atomics and Locks》第 3 章（内存序直觉跨语言通用）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target F1_release_acquire --config Release
//     ./build-vs2026/F1_release_acquire/Release/F1_release_acquire.exe
//
//   说明：内存序问题在 x86（强内存模型）上往往“恰好正确”而隐藏。
//   本题用打印展示“正确配对”的逻辑，并在注释里讲清在弱内存模型
//   （ARM/POWER）或经编译器重排后，错误写法为何会真实暴露。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

// =====================================================================
// 被发布的“数据载荷”：注意它们是【非原子】的普通变量。
//   能否安全地跨线程读到它们，完全依赖 ready 标志的 release/acquire 配对
//   建立的 happens-before 关系——而不是把它们自己变成 atomic。
//   这正是 release-acquire 的核心价值：用一个原子标志，
//   把“它之前写的一整片普通数据”安全地交付给读到该标志的线程。
// =====================================================================
struct Payload {
    int          a = 0;
    int          b = 0;
    std::string  note;
};

Payload            g_payload;            // 非原子 payload（生产方写、消费方读）
std::atomic<bool>  g_ready{false};       // 发布标志（原子）

// =====================================================================
// 必做 1：release 写标志 / acquire 读标志，安全发布非原子 payload。
//
//   生产方：先写好 payload 的每个字段（这些写在 release 写之前，
//           即 sequenced-before），最后用 release 写把 ready 置 true。
//   消费方：自旋 acquire 读 ready，直到读到 true；此时它与生产方那次
//           release 写建立 synchronizes-with，于是生产方在 release 之前的
//           全部写 happens-before 消费方在 acquire 之后的读——
//           可以安全读 payload，保证看到完整、最新的值。
// =====================================================================
void producer() {
    cs::logf("[producer] 写入非原子 payload（a/b/note）…");
    // ----- 这些都是 sequenced-before 下面那次 release 写的普通写 -----
    g_payload.a = 42;
    g_payload.b = 7;
    g_payload.note = "hello from producer";

    // TODO [必做 1]: 用 release 写把 ready 置 true，发布上面整片数据。
    //   要点：必须用 std::memory_order_release。
    //   release 写的语义：保证“本线程在这次写之前的所有读写”
    //   不会被重排到这次写之后，并为读到该值的 acquire 读提供同步点。
    //   下面已是正确写法，留作必做 1 的参考实现：
    g_ready.store(true, std::memory_order_release);
    cs::logf("[producer] 已 release 写 ready=true（数据已发布）。");
}

void consumer() {
    cs::logf("[consumer] acquire 自旋等待 ready=true…");

    // TODO [必做 1]: 用 acquire 读自旋等待 ready 变 true。
    //   要点：必须用 std::memory_order_acquire。
    //   acquire 读的语义：一旦读到 release 写进去的值，
    //   就与那次 release 写 synchronizes-with；本线程在这次读之后的
    //   读写不会被重排到这次读之前。于是下面读 payload 是安全的。
    //   下面已是正确写法，留作必做 1 的参考实现：
    while (!g_ready.load(std::memory_order_acquire)) {
        std::this_thread::yield(); // 让出 CPU，避免忙等占满核
    }

    // 走到这里：已与 producer 的 release 写建立 happens-before。
    // 读非原子 payload 不构成数据竞争，且保证看到 producer 写入的完整值。
    cs::logf("[consumer] 读到 ready，安全读取 payload："
             "a=", g_payload.a, " b=", g_payload.b, " note=", g_payload.note);
}

void demo_release_acquire() {
    cs::println("======== 必做 1：release/acquire 安全发布非原子 payload ========");
    g_ready.store(false, std::memory_order_relaxed);
    g_payload = Payload{};

    // 故意让 consumer 先起跑（它会 acquire 自旋等待），
    // producer 稍后写数据并 release 发布。
    std::thread tc(consumer);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::thread tp(producer);

    tp.join();
    tc.join();

    cs::println("");
    cs::println("happens-before 边（本练习要你能画出来）：");
    cs::println("  producer: 写 a  -- sequenced-before --> 写 b");
    cs::println("            写 b  -- sequenced-before --> 写 note");
    cs::println("            写 note -- sequenced-before --> store(ready, release)");
    cs::println("  store(ready, release) -- synchronizes-with --> load(ready, acquire)==true");
    cs::println("  load(ready, acquire)  -- sequenced-before --> 读 a/b/note");
    cs::println("  传递闭包 => producer 的全部写 happens-before consumer 的全部读");
    cs::println("");
}

// =====================================================================
// 进阶 1：把 acquire 换成 relaxed，说明为何可能读到未初始化/陈旧数据。
//
//   理论要点（这是本题最硬核的一句）：
//     relaxed 读【不】与 producer 的 release 写建立 synchronizes-with，
//     因此【不】产生 happens-before。于是 consumer 对非原子 payload 的读
//     与 producer 对它的写【没有先后关系】——这构成数据竞争（data race），
//     按标准是【未定义行为（UB）】。即便 ready 读到了 true，
//     payload 的各字段也【不保证】已对本线程可见：
//       - 编译器可能把 payload 的写重排到 ready 写之后；
//       - 弱内存模型 CPU（ARM/POWER）可能让其他核延迟看到 payload 的写；
//     结果就是“看到 ready=true，却读到 a/b 还是 0、note 还是空”——
//     即读到了“未初始化/半initialized”数据。
//
//   为什么在你机器上（x86）可能“看不出错”：
//     x86 是强内存模型（TSO），store-store / load-load 不会被硬件重排，
//     普通 store 本身近似带 release 语义，普通 load 近似带 acquire 语义。
//     因此 relaxed 在 x86 上往往“碰巧正确”。但这只是平台运气，
//     【代码依然是 UB】：换 ARM、或开高优化让编译器重排，就会真实崩坏。
//     学习内存序的纪律是：按标准的 happens-before 推理，而非按某台机器的表现。
// =====================================================================
void demo_relaxed_is_wrong() {
    cs::println("======== 进阶 1：把 acquire 换成 relaxed —— 为何是 UB ========");

    g_ready.store(false, std::memory_order_relaxed);
    g_payload = Payload{};

    std::thread tc([] {
        cs::logf("[consumer-relaxed] relaxed 自旋等待 ready…（注意：这是错误写法）");

        // TODO [进阶 1]: 这里【故意】用 relaxed 读 ready，演示错误。
        //   要点：relaxed 只保证 ready 这一个变量的原子性与修改顺序，
        //   【不】与 producer 的 release 写建立同步，【不】产生 happens-before。
        //   于是下面读非原子 payload 与 producer 的写构成数据竞争 = UB。
        //   下面这行就是“反面教材”的核心，保留以供演示与讲解：
        while (!g_ready.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
        }

        // 危险：理论上可能读到 a=0,b=0,note=""（未初始化/陈旧值），
        // 因为没有任何 happens-before 保证 payload 的写已可见。
        cs::logf("[consumer-relaxed] 读到 payload："
                 "a=", g_payload.a, " b=", g_payload.b, " note='", g_payload.note, "'");
        cs::logf("[consumer-relaxed] 说明：在 x86 上常常‘碰巧正确’，"
                 "但在 ARM/POWER 或编译器重排下可能读到未初始化数据——本写法是 UB。");

        // 正确做法仍然是 acquire（注释保留，提醒应当怎么写）：
        //   while (!g_ready.load(std::memory_order_acquire)) { ... }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::thread tp([] {
        g_payload.a = 99;
        g_payload.b = 100;
        g_payload.note = "published with release";
        g_ready.store(true, std::memory_order_release); // 生产方仍正确用 release
        cs::logf("[producer-relaxed] 已 release 发布 payload。");
    });

    tp.join();
    tc.join();
    cs::println("");
}

int main() {
    cs::println("==== F1_release_acquire：release-acquire 安全发布非原子数据 ====\n");

    demo_release_acquire();   // 必做 1：release/acquire 配对，安全发布 payload
    demo_relaxed_is_wrong();  // 进阶 1：换成 relaxed，讲清为何是 UB

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
