// =====================================================================
// 练习 F-4：发布-订阅内存序模式（版本号发布一整片数据）
//   对应文档：Concurrency_Study/08-模块F-内存模型与memory_order.md 的 练习 F-4
//
//   学习目标：
//     - 把 F-1 的“一个 bool 标志发布一个 payload”推广为通用模式：
//       单生产者反复发布【新版本】的一整块结构体/缓冲，
//       用一个原子【版本号（version / sequence number）】作为发布点；
//     - 生产者：先写好新数据，再用 release 写把版本号 +1（发布）；
//       消费者：用 acquire 读版本号，读到新版本后再读数据，
//       acquire 读与 release 写建立 synchronizes-with → happens-before，
//       保证读到的是与该版本号配套的完整数据；
//     - 体会这正是无锁（lock-free）数据结构的基础：
//       “先把不可见的新状态准备好，再用一次 release 原子写让它一举可见”
//       是几乎所有无锁发布/单写多读结构的共同骨架。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/memory_order
//       （release-acquire ordering；release sequence 一节）
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3 / 第 7 章
//     - Mara Bos, 《Rust Atomics and Locks》第 3 章（release/acquire 一节）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target F4_publish_pattern --config Release
//     ./build-vs2026/F4_publish_pattern/Release/F4_publish_pattern.exe
//
//   说明：本题用“单生产者多次发布递增版本号 + 多消费者各自追版本”
//   演示通用发布模式。注意这里的发布数据是【非原子】结构体，
//   它的可见性完全由版本号的 release/acquire 配对担保。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

// =====================================================================
// 被发布的“一整片数据”：非原子结构体。
//   它能否被消费者安全读到，取决于 g_version 的 release/acquire 配对。
// =====================================================================
struct Snapshot {
    int         id    = 0;     // 该版本携带的业务数据之一
    long long   value = 0;     // 之二
    std::string label;         // 之三（变长，最能暴露“读到半成品”）
};

Snapshot               g_data;                 // 非原子，被版本号担保可见
std::atomic<long long> g_version{0};            // 版本号（发布点，原子）

constexpr long long    kVersionsToPublish = 5;  // 生产者发布的版本数

// =====================================================================
// 必做 1：版本号发布。
//   生产者每次：先写好新一版 g_data，再用 release 写把版本号 +1；
//   消费者每次：acquire 读版本号，发现比自己上次见过的大，就读 g_data。
//
//   关键不变式（消费者据此推理）：
//     “我用 acquire 读到了版本号 V”
//     => 与“生产者用 release 写出 V 的那次 store” synchronizes-with
//     => 生产者在那次 store 之前对 g_data 的全部写 happens-before
//        我在 acquire 读之后对 g_data 的读
//     => 我读到的 g_data 必然是与版本 V 配套的完整数据。
// =====================================================================
void producer() {
    for (long long v = 1; v <= kVersionsToPublish; ++v) {
        // ---- 先准备好新一版数据（这些写 sequenced-before 下面的 release 写）----
        g_data.id    = static_cast<int>(v);
        g_data.value = v * 1000 + v; // 一个可校验的规律值
        g_data.label = "snapshot-v" + std::to_string(v);

        // TODO [必做 1]: 用 release 写把版本号设为 v，发布这一整片数据。
        //   要点：release 写保证“本次写之前对 g_data 的全部写”
        //   对随后 acquire 读到该版本号的消费者可见。
        //   下面已是正确写法，留作必做 1 的参考实现：
        g_version.store(v, std::memory_order_release);

        cs::logf("[producer] 已发布版本 v=", v, "（release 写版本号）。");
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
}

void consumer(int cid) {
    long long last_seen = 0;
    while (last_seen < kVersionsToPublish) {
        // TODO [必做 1]: 用 acquire 读版本号。
        //   要点：acquire 读一旦读到生产者 release 写进去的版本号，
        //   就与那次 release 写 synchronizes-with → happens-before，
        //   使随后对 g_data 的读保证看到与该版本配套的完整数据。
        //   下面已是正确写法，留作必做 1 的参考实现：
        long long v = g_version.load(std::memory_order_acquire);

        if (v > last_seen) {
            // 读到新版本：此处读 g_data 受 happens-before 担保，安全且完整。
            // 注意：单写者下，若我们读得慢，可能直接跳到最新版（跳过中间版本），
            // 这正常——我们要的是“看到的那一版数据是自洽完整的”，而非每版都见。
            const int       id    = g_data.id;
            const long long value = g_data.value;
            const std::string label = g_data.label;

            // 自洽性校验：value 应等于 id*1000+id，label 应含对应版本号。
            const bool consistent =
                (value == static_cast<long long>(id) * 1000 + id) &&
                (label == "snapshot-v" + std::to_string(id));

            cs::logf("[consumer ", cid, "] 看到版本 v=", v,
                     " data{id=", id, ", value=", value, ", label=", label,
                     "} 自洽? ", consistent);

            last_seen = v;
        } else {
            std::this_thread::yield(); // 还没新版本，让出 CPU
        }
    }
    cs::logf("[consumer ", cid, "] 已追到最新版本 ", kVersionsToPublish, "，退出。");
}

void demo_version_publish() {
    cs::println("======== 必做 1：版本号 release/acquire 发布一整片数据 ========");

    g_version.store(0, std::memory_order_relaxed);
    g_data = Snapshot{};

    constexpr int kConsumers = 3;
    std::vector<std::thread> consumers;
    for (int i = 0; i < kConsumers; ++i) consumers.emplace_back(consumer, i);

    std::thread prod(producer);

    prod.join();
    for (auto& c : consumers) c.join();

    cs::println("");
    cs::println("这就是无锁结构的发布骨架：");
    cs::println("  1) 先把新状态（整片数据）在‘别人看不到’时准备好；");
    cs::println("  2) 用一次 release 原子写（版本号 / 指针 / 标志）一举发布；");
    cs::println("  3) 读者 acquire 读到该发布点，即 happens-before 全部新数据。");
    cs::println("  =>（进阶方向）若发布的是‘指向新数据的指针’，配合 RCU / 引用计数，");
    cs::println("     就是 read-mostly 无锁结构的标准做法。");
    cs::println("");
}

int main() {
    cs::println("==== F4_publish_pattern：版本号发布-订阅内存序模式 ====\n");

    demo_version_publish(); // 必做 1：版本号 release/acquire 发布整片数据

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
