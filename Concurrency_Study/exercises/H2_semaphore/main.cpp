// =====================================================================
// 练习 H-2：counting / binary semaphore（计数信号量 / 二值信号量）
//   对应文档：Concurrency_Study/10-模块H-高级同步原语.md 的 练习 H-2
//
//   学习目标：
//     - 掌握 std::counting_semaphore<N>（计数信号量，<semaphore>，C++20）：
//       内部是一个非负计数，acquire() 把计数 -1（为 0 则阻塞），
//       release() 把计数 +1（并唤醒等待者）。模板参数 N 是计数上限（LeastMaxValue）。
//       用它做【限流】：最多允许 K 个线程同时进入临界区 / 持有资源池里的资源；
//     - 掌握 std::binary_semaphore（二值信号量，= counting_semaphore<1>）：
//       计数只在 0/1 之间，常用作线程间“一次性信号 / 唤醒”的轻量手段；
//     - 区分 semaphore 与 mutex：mutex 有“所有权”（谁锁谁解），且天生 1 份；
//       semaphore 没有所有权概念——可以 A 线程 acquire、B 线程 release，
//       计数可 > 1，因此适合“资源计数 / 信号传递”而非“互斥保护一段代码”；
//     - 会用 try_acquire() / try_acquire_for() 做非阻塞 / 限时获取。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/counting_semaphore
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章
//       （4.x 信号量一节）
//     - 提案 P1135R6（The C++20 Synchronization Library）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target H2_semaphore --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <semaphore>
#include <thread>
#include <vector>

// =====================================================================
// 第一部分：counting_semaphore<K> 做资源池信号量（限流）
//
//   场景：有一个容量为 K 的资源池（比如 K 条数据库连接 / K 个许可），
//   N 个线程（N > K）争用。任意时刻最多允许 K 个线程“持有资源”，其余阻塞等待。
//
//   做法：counting_semaphore<K> pool(K)：初始计数 = K（K 份可用资源）。
//     - 进区：pool.acquire() —— 计数 -1；若已为 0（资源用尽）则阻塞，
//       直到别人 release。
//     - 出区：pool.release() —— 计数 +1，唤醒一个等待者。
//   用一个原子计数器实时统计“当前在区内的线程数”，断言它【从不超过 K】。
// =====================================================================
void demo_counting_semaphore() {
    cs::println("\n---- 第一部分：counting_semaphore<K> 资源池限流 ----");

    constexpr std::ptrdiff_t kPermits = 3;  // 资源池容量 K（同时最多 3 个）
    constexpr int kThreads = 8;             // 争用线程数 N（> K）

    // 模板参数是计数上限（LeastMaxValue）；构造参数是初始计数。
    std::counting_semaphore<kPermits> pool(kPermits);

    std::atomic<int> in_region{0};  // 当前在区内的线程数（用于验证 <= K）
    std::atomic<int> max_seen{0};   // 观察到的峰值占用
    std::atomic<bool> violated{false};

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i, kPermits, &pool, &in_region, &max_seen, &violated] {
            // TODO [必做 1]: 用 counting_semaphore 实现资源池信号量。
            //   进入资源区前 acquire（计数 -1，用尽则阻塞）；
            //   离开资源区后 release（计数 +1，唤醒等待者）。
            //   要求：临界区内的并发数从不超过 kPermits。
            //   完成后请遮住下面参考实现重写一遍。
            //
            //   参考实现（已启用以保证可编译运行）：
            pool.acquire(); // 申请一份资源：计数 -1；为 0 则阻塞等待。

            // ===== 资源区（最多 K 个线程能同时到这里）=====
            const int now = in_region.fetch_add(1, std::memory_order_relaxed) + 1;
            // 维护峰值（CAS 抬高 max_seen）。
            int prev = max_seen.load(std::memory_order_relaxed);
            while (now > prev &&
                   !max_seen.compare_exchange_weak(prev, now,
                                                   std::memory_order_relaxed)) {
            }
            if (now > static_cast<int>(kPermits)) {
                violated.store(true, std::memory_order_relaxed); // 越界（不该发生）
            }
            cs::logf("[thread ", i, "] 进入资源区，当前占用 = ", now,
                     "（上限 ", kPermits, "）。");

            std::this_thread::sleep_for(std::chrono::milliseconds(30)); // 模拟用资源

            in_region.fetch_sub(1, std::memory_order_relaxed);
            // ===== 离开资源区 =====

            pool.release(); // 归还资源：计数 +1，唤醒一个等待者。
            cs::logf("[thread ", i, "] 离开资源区，已 release。");
        });
    }

    for (auto& th : threads) th.join();

    cs::logf("[main] 峰值同时占用 = ", max_seen.load(), "，限额 = ", kPermits, " -> ",
             (max_seen.load() <= static_cast<int>(kPermits) && !violated.load()
                  ? "未越界 OK"
                  : "越界 VIOLATION"));

    // 进阶提示（见文档“进阶任务”）：
    //   - try_acquire()：非阻塞地尝试拿资源，拿不到立即返回 false（用于“拿不到就干别的”）。
    //   - try_acquire_for(timeout)：限时等待，超时返回 false（用于带超时的限流）。
    cs::println("  要点：counting_semaphore 无所有权，可 A 线程 acquire、B 线程 release。");
}

// =====================================================================
// 第二部分：binary_semaphore 做一次性 ping-pong 通知
//
//   binary_semaphore == counting_semaphore<1>：计数只在 0/1 间。
//   常用作线程间“信号 / 唤醒”：一方 release（“信号已就绪”），另一方 acquire（等信号）。
//
//   本例：ping 线程先工作，做完后 release 信号量通知 pong；pong 线程一直
//   acquire 等这个信号，收到后才继续。这就是一次性的“握手 / 交棒”。
//   注意：semaphore 没有所有权——release 的线程和 acquire 的线程可以不同，
//   这正是它比 mutex 更适合“跨线程发信号”的原因。
// =====================================================================
void demo_binary_semaphore() {
    cs::println("\n---- 第二部分：binary_semaphore 一次性 ping-pong 通知 ----");

    // 初始 0：表示“信号尚未就绪”，pong 一来就会阻塞在 acquire 上。
    std::binary_semaphore signal_ready(0);
    // 第二个信号量用于反向通知（pong 收到后回告 ping），演示双向交棒。
    std::binary_semaphore signal_ack(0);

    std::atomic<int> shared_payload{0};

    std::thread ping([&] {
        cs::logf("[ping] 正在准备数据……");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        shared_payload.store(42, std::memory_order_relaxed);

        // TODO [必做 2]: 用 binary_semaphore 做一次性通知。
        //   ping 准备好数据后 release(signal_ready)，通知 pong“可以来取了”；
        //   随后 acquire(signal_ack) 等 pong 回告，演示双向交棒。
        //   完成后请遮住下面参考实现重写一遍。
        //
        //   参考实现（已启用以保证可编译运行）：
        cs::logf("[ping] 数据就绪，发信号 release(signal_ready)。");
        signal_ready.release();   // 通知 pong：信号就绪（计数 0->1）。

        signal_ack.acquire();     // 等 pong 的回告（阻塞到 pong release）。
        cs::logf("[ping] 收到 pong 的回告，结束。");
    });

    std::thread pong([&] {
        cs::logf("[pong] 等待 ping 的信号 acquire(signal_ready)……");
        signal_ready.acquire();   // 阻塞到 ping release（计数 1->0）。
        // 收到信号后再读 payload（此处用 relaxed 仅为演示；真实可见性可结合
        // 模块 F 的 release/acquire——semaphore 的 release/acquire 本身也建立同步）。
        cs::logf("[pong] 收到信号！读到 payload = ",
                 shared_payload.load(std::memory_order_relaxed));

        cs::logf("[pong] 回告 ping：release(signal_ack)。");
        signal_ack.release();     // 回告 ping（计数 0->1）。
    });

    ping.join();
    pong.join();
    cs::println("  要点：semaphore 适合“跨线程发信号”——release/acquire 可由不同线程执行。");
}

int main() {
    cs::println("==== H2_semaphore：counting（限流）/ binary（信号）信号量 ====");

    demo_counting_semaphore();
    demo_binary_semaphore();

    cs::println("\n小结：");
    cs::println("  - counting_semaphore<K>：非负计数，acquire(-1)/release(+1)；");
    cs::println("    做资源池限流——任意时刻最多 K 个线程持有资源。无所有权。");
    cs::println("  - binary_semaphore（=counting_semaphore<1>）：0/1 计数，");
    cs::println("    做线程间一次性信号/唤醒；release 与 acquire 可由不同线程执行。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
