// =====================================================================
// 练习 I-1：atomic<shared_ptr>（原子共享指针）
//   对应文档：Concurrency_Study/11-模块I-安全内存回收.md 的 练习 I-1
//
//   学习目标：
//     - 认清无锁结构的【回收难题（reclamation problem）】：线程 A 读到指向
//       节点 N 的指针、还没解引用时，线程 B 把 N 摘下并 delete → A 解引用悬垂
//       指针，即 use-after-free。这正是模块 G Treiber 栈“pop 故意泄漏”的
//       真正病根。
//     - 用 C++20 的 std::atomic<std::shared_ptr<T>>（<memory>）把回收交给
//       【引用计数（reference counting）】：只要还有人持有 shared_ptr，对象
//       就不会析构。读者 load 出一份 shared_ptr 副本，引用计数 +1，从此哪怕
//       写者 store 了新值、丢掉了对旧对象的最后一个外部引用，旧对象也不会被
//       提前销毁——读者手里的副本撑着它，读完副本析构、计数归零才真正释放。
//       use-after-free 被根除。
//     - 体会代价：std::atomic<std::shared_ptr<T>> 在【多数标准库实现里并非
//       lock-free】——内部往往用一把锁（或分片锁）来原子地操作“控制块指针 +
//       引用计数”这两样东西。所以它安全、好写，但有同步开销，热路径上的吞吐
//       不如 hazard pointer / RCU。本题最后会把它和无保护的裸做法对比，让你
//       亲手感受“安全是有价的”。
//
//   官方参考：
//     - cppreference std::atomic<std::shared_ptr>：
//       https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic2
//     - cppreference std::shared_ptr：
//       https://en.cppreference.com/w/cpp/memory/shared_ptr
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams)
//       第 7 章（无锁数据结构的内存回收）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target I1_atomic_shared_ptr --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------
// 一个“可被并发安全读/写的共享配置单元”。
//   写者用一个全新的 Config 原子地替换旧的；读者随时 load 出当前 Config 用。
//   回收完全交给 shared_ptr 的引用计数——这正是本题要演示的安全回收方式。
// ---------------------------------------------------------------------
struct Config {
    int version = 0;
    int payload = 0;
};

class AtomicConfigCell {
public:
    AtomicConfigCell()
        : cell_(std::make_shared<Config>()) {}

    // 读：load 出当前 Config 的一份 shared_ptr 副本（引用计数 +1）。
    //   返回副本后，即便别的线程立刻 store 了新值，这份旧对象也被本副本撑着，
    //   读者解引用它绝不会 use-after-free。
    std::shared_ptr<const Config> read() const {
        // TODO [必做 1]: 用 std::atomic<std::shared_ptr<Config>>::load 读出当前值。
        //   参考实现已给出以保证可编译运行；理解后请遮住重写一遍。
        //   关键：load 返回的是一份【独立的 shared_ptr 副本】，引用计数被安全地 +1，
        //   这一步（读副本+增计数）由 atomic<shared_ptr> 原子完成，不会撕裂。
        //
        //   return cell_.load(std::memory_order_acquire);
        return cell_.load(std::memory_order_acquire);
    }

    // 写：用新的 Config 原子地替换旧的。旧对象的释放由引用计数决定——
    //   当最后一个持有它的 shared_ptr（可能是某个还没读完的读者）析构时才释放。
    void write(std::shared_ptr<Config> new_cfg) {
        // TODO [必做 1]: 用 std::atomic<std::shared_ptr<Config>>::store 发布新值。
        //   参考实现已给出。release 与读者的 acquire 配对，保证读者看到的 Config
        //   内容是写者完整写好的。
        //
        //   cell_.store(std::move(new_cfg), std::memory_order_release);
        cell_.store(std::move(new_cfg), std::memory_order_release);
    }

private:
    // C++20：<memory> 提供 std::atomic<std::shared_ptr<T>> 特化。
    //   它把“拷贝控制块指针 + 调整引用计数”这组复合操作做成原子的。
    //   ★ 注意：多数实现【并非 lock-free】（is_lock_free() 通常为 false），
    //     内部往往加锁。这是它和裸指针 + hazard pointer/RCU 的关键代价差异。
    std::atomic<std::shared_ptr<Config>> cell_;
};

int main() {
    cs::println("==== I1_atomic_shared_ptr：用引用计数解决无锁回收难题 ====\n");

    // ---- 先点明 lock-free 性质（多数实现为 false）----
    {
        std::atomic<std::shared_ptr<Config>> probe{std::make_shared<Config>()};
        cs::logf("[probe] atomic<shared_ptr> 是否 lock-free？ -> ",
                 (probe.is_lock_free() ? "true（少见）" : "false（典型：内部加锁，有代价）"));
    }

    AtomicConfigCell cell;

    constexpr int kReaders = 4;
    constexpr int kWriters = 2;
    constexpr int kWritesPerWriter = 20000;
    constexpr int kReadsPerReader = 50000;

    std::atomic<bool> start{false};
    std::atomic<long long> total_reads{0};
    std::atomic<int> max_version_seen{0};

    // 必做 1（验收）：并发读写不崩、不 use-after-free。
    //   读者反复 read() 出 shared_ptr 副本并解引用其内容；
    //   写者反复用新版本 store 替换。引用计数保证读者手里的对象永不被提前释放。
    std::vector<std::thread> threads;

    for (int w = 0; w < kWriters; ++w) {
        threads.emplace_back([&, w] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int i = 0; i < kWritesPerWriter; ++i) {
                auto cfg = std::make_shared<Config>();
                cfg->version = w * kWritesPerWriter + i + 1;
                cfg->payload = cfg->version * 2; // 设个与 version 强相关的不变量供读者校验
                cell.write(std::move(cfg));
            }
            cs::logf("[writer ", w, "] 完成 ", kWritesPerWriter, " 次发布。");
        });
    }

    for (int r = 0; r < kReaders; ++r) {
        threads.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            long long local = 0;
            int local_max = 0;
            for (int i = 0; i < kReadsPerReader; ++i) {
                std::shared_ptr<const Config> snap = cell.read();
                // 解引用 snap：若没有引用计数撑着，这里极易 use-after-free。
                // 校验不变量 payload == version*2，撕裂/悬垂会让它不成立。
                const int v = snap->version;
                const int p = snap->payload;
                if (p == v * 2) {
                    ++local;
                }
                if (v > local_max) local_max = v;
            }
            total_reads.fetch_add(local, std::memory_order_relaxed);
            // 更新全局最大版本（CAS 循环）
            int cur = max_version_seen.load(std::memory_order_relaxed);
            while (local_max > cur &&
                   !max_version_seen.compare_exchange_weak(cur, local_max,
                                                           std::memory_order_relaxed)) {
            }
        });
    }

    start.store(true, std::memory_order_release);
    for (auto& th : threads) th.join();

    const long long expected_reads =
        static_cast<long long>(kReaders) * kReadsPerReader;
    cs::logf("[main] 一致读取（payload==version*2）次数：", total_reads.load(),
             " / 总读取 ", expected_reads, " -- ",
             (total_reads.load() == expected_reads ? "全部一致 OK（无撕裂/无悬垂）"
                                                   : "出现不一致 MISMATCH"));
    cs::logf("[main] 读者观察到的最大版本号：", max_version_seen.load());

    cs::println("\n要点：");
    cs::println("  - 回收难题：A 读到指针、未解引用时 B delete 之 -> use-after-free。");
    cs::println("  - 解法：atomic<shared_ptr> 用引用计数撑住读者手里的旧对象，永不提前释放。");
    cs::println("  - 代价：多数实现非 lock-free（内部加锁），吞吐不如 hazard pointer/RCU。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
