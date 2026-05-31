// =====================================================================
// 练习 I-3：RCU 读-拷贝-更新（read-copy-update）
//   对应文档：Concurrency_Study/11-模块I-安全内存回收.md 的 练习 I-3
//
//   学习目标：
//     - 用 RCU 实现一个【读多写少】的共享配置：读者侧几乎零开销（不加锁、不写
//       共享争用热点，只标记一下“我在读临界区”），写者侧 publish 新版本 +
//       retire 旧版本，等宽限期（grace period）——所有当前读者都离开后——
//       才安全回收旧版本。
//     - 掌握读侧协议：rcu_domain::lock/unlock 包住“读取指针 + 解引用使用”的
//       整个临界区；在临界区内读到的指针，保证在临界区结束前不会被回收。
//     - 掌握写侧协议：构造新版本 -> 原子 publish（替换指针）-> rcu_retire 旧版本
//       -> rcu_synchronize 等宽限期结束 -> 旧版本被安全回收。
//     - 对比 hazard pointer（I-2）：hazard pointer 精确登记“我在用哪个指针”，
//       RCU 用更粗的“等老读者全离开”的宽限期；RCU 读侧更便宜，但回收延迟更
//       依赖最慢的读者。
//
//     ★ 本题使用本仓库自带的【教学版】RCU（cs::rcu_*，concurrency_study/rcu.hpp）。
//       C++26 标准在 std 命名空间 <rcu> 提供等价设施（提案 P2545R4），但 MSVC
//       VS2026 尚未实现，故用 cs:: 教学版。两者 API 差异映射见模块文档对照表。
//
//   官方参考：
//     - 提案 P2545R4 "Read-Copy Update (RCU)"：
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2545r4.pdf
//     - 早期提案 P0566
//     - cppreference（C++26）<rcu>：
//       https://en.cppreference.com/w/cpp/header/rcu
//     - folly RCU（工业级对照）：
//       https://github.com/facebook/folly/blob/main/folly/synchronization/Rcu.h
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target I3_rcu --config Release
// =====================================================================
#include "concurrency_study/log.hpp"
#include "concurrency_study/rcu.hpp"

#include <atomic>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------
// 受 RCU 保护的共享配置。继承 cs::rcu_obj_base 以获得 retire() 能力
//   （也可以用非侵入式的 cs::rcu_retire(ptr)，本题两种都演示）。
//
//   不变量：checksum == a + b。读者每次读都校验它，撕裂/悬垂会让它不成立。
// ---------------------------------------------------------------------
struct Config : cs::rcu_obj_base<Config> {
    int version = 0;
    int a = 0;
    int b = 0;
    int checksum = 0; // 应恒等于 a + b
};

// 全局当前配置指针（写者 publish 新版本时原子替换它）。
std::atomic<Config*> g_config{nullptr};

int main() {
    cs::println("==== I3_rcu：读多写少共享配置的 RCU 安全回收 ====\n");

    // 初始版本。
    {
        Config* init = new Config();
        init->version = 0;
        init->a = 1;
        init->b = 1;
        init->checksum = init->a + init->b;
        g_config.store(init, std::memory_order_release);
    }

    constexpr int kReaders = 6;       // 读多
    constexpr int kWriters = 1;       // 写少
    constexpr int kReadsPerReader = 200000;
    constexpr int kWritesTotal = 2000;

    std::atomic<bool> stop{false};
    std::atomic<bool> start{false};
    std::atomic<long long> consistent_reads{0};

    std::vector<std::thread> threads;

    // --------- 读者：rcu 读侧（必做 1 的读侧部分）---------
    for (int r = 0; r < kReaders; ++r) {
        threads.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            long long local_ok = 0;
            for (int i = 0; i < kReadsPerReader && !stop.load(std::memory_order_relaxed); ++i) {
                // TODO [必做 1]: 用 rcu 读侧临界区包住“读指针 + 解引用”。
                //   参考实现已给出以保证可编译运行；理解后请遮住重写一遍。
                //   关键：进入临界区（lock）后读到的 Config*，保证在离开临界区（unlock）
                //   前不会被写者回收——这就是读者无需加锁也不会 use-after-free 的原因。
                //
                //   cs::rcu_domain& dom = cs::rcu_default_domain();
                //   dom.lock();
                //   Config* cfg = g_config.load(std::memory_order_acquire);
                //   bool ok = (cfg->checksum == cfg->a + cfg->b);
                //   dom.unlock();
                cs::rcu_domain& dom = cs::rcu_default_domain();
                dom.lock(); // 进入读临界区
                Config* cfg = g_config.load(std::memory_order_acquire);
                // 在临界区内解引用：受 RCU 保护，cfg 不会被提前回收。
                const bool ok = (cfg->checksum == cfg->a + cfg->b);
                dom.unlock(); // 离开读临界区
                if (ok) ++local_ok;
            }
            consistent_reads.fetch_add(local_ok, std::memory_order_relaxed);
        });
    }

    // --------- 写者：publish 新版本 + retire 旧版本（必做 1 的写侧部分）---------
    for (int w = 0; w < kWriters; ++w) {
        threads.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int i = 1; i <= kWritesTotal; ++i) {
                // 1) 复制-修改：构造一个全新的版本（read-COPY-update 的 copy）。
                Config* fresh = new Config();
                fresh->version = i;
                fresh->a = i;
                fresh->b = i * 3;
                fresh->checksum = fresh->a + fresh->b; // 维持不变量

                // 2) publish：原子地把全局指针换成新版本（release 与读者 acquire 配对）。
                Config* old = g_config.exchange(fresh, std::memory_order_acq_rel);

                // TODO [必做 1]: retire 旧版本（不立即 delete），随后等宽限期回收。
                //   参考实现已给出。两种等价写法：
                //     (a) 侵入式：old->retire();              // Config 继承了 rcu_obj_base
                //     (b) 非侵入式：cs::rcu_retire(old);      // 任意指针都行
                //   关键：此刻可能仍有读者正握着 old 在它们的读临界区里使用，
                //   所以【不能直接 delete old】，必须 retire 延迟回收。
                if (old) {
                    old->retire(); // 等价于 cs::rcu_retire(old);
                }

                // 3) 等一个宽限期：确保“拿到 old 的老读者”都已离开读临界区，
                //    然后回收已 retire 的旧版本。rcu_barrier 在此实际执行删除。
                //    （演示用：每隔若干次写做一次屏障，攒批回收更接近真实用法。）
                if ((i % 16) == 0) {
                    cs::rcu_barrier(); // 等宽限期 + 回收已退休的旧版本
                }
            }
            cs::logf("[writer] 完成 ", kWritesTotal, " 次 publish + retire。");
        });
    }

    start.store(true, std::memory_order_release);

    // 让读者多跑一会儿；写者完成后停止读者。
    // 写者线程是 threads 里的最后 kWriters 个；先等它们结束。
    for (int idx = kReaders; idx < kReaders + kWriters; ++idx) {
        threads[idx].join();
    }
    stop.store(true, std::memory_order_release);
    for (int idx = 0; idx < kReaders; ++idx) {
        threads[idx].join();
    }

    // 收尾：回收所有剩余退休对象 + 删除最后 publish 的版本。
    cs::rcu_barrier();
    Config* last = g_config.exchange(nullptr, std::memory_order_acq_rel);
    delete last; // 此刻已无并发读者，安全直接 delete

    cs::logf("[main] 一致读取（checksum==a+b）次数：", consistent_reads.load(),
             " -- 若期间无 use-after-free，则不变量应【始终成立】");
    cs::logf("[main] 全部一致：",
             (consistent_reads.load() > 0 ? "是（读者全程读到自洽版本）" : "无有效读取"));

    cs::println("\n要点：");
    cs::println("  - 读侧：rcu_domain::lock/unlock 包住读临界区，读到的指针在区内不被回收。");
    cs::println("  - 写侧：复制新版本 -> 原子 publish -> retire 旧版本 -> 宽限期后回收。");
    cs::println("  - 对比 hazard pointer：RCU 读侧更便宜，但回收要等最慢的读者离开宽限期。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
