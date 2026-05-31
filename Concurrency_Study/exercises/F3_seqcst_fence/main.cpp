// =====================================================================
// 练习 F-3：seq_cst 与内存栅栏（修复 store-load 重排）
//   对应文档：Concurrency_Study/08-模块F-内存模型与memory_order.md 的 练习 F-3
//
//   学习目标：
//     - 复现经典的 store-load 重排现象（Dekker / store-buffer 模式）：
//       两个线程各自先 store 自己的标志、再 load 对方的标志，
//       在 relaxed / 甚至 acquire-release 下，两者【可能都读到旧值】；
//     - 用 memory_order_seq_cst 修复：seq_cst 在 acquire/release 之外，
//       额外提供“所有 seq_cst 操作的单一全序（single total order）”，
//       这条全序禁止“两边都看不到对方写”的结果；
//     - 用 std::atomic_thread_fence(seq_cst) 替代“每次操作都 seq_cst”，
//       把屏障从“每个变量”上移到“线程的关键点”上，达到同样的修复效果。
//
//   为什么 acquire/release 不足以修这个：
//     release/acquire 只约束“一个线程在某原子上的写”与“另一个线程读到它”
//     之间的可见性（message passing 方向）。
//     但 store-load 重排是【同一线程内】“我先写 x，再读 y”被观测成
//     “先读 y（旧值），后写 x”——这是 StoreLoad 重排，release/acquire
//     并不禁止它。只有 seq_cst 的全序（或一道 seq_cst 全屏障）能禁止。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/memory_order
//       （seq_cst ordering；及 std::atomic_thread_fence 一节）
//     - https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3.3
//     - Herb Sutter, "atomic<> Weapons"（讲透 SC 全序与 StoreLoad 重排）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target F3_seqcst_fence --config Release
//     ./build-vs2026/F3_seqcst_fence/Release/F3_seqcst_fence.exe
//
//   x86 提示：x86 唯一允许的硬件重排恰好就是 StoreLoad，因此本现象
//   在 x86 上【可能】真实复现（relaxed 版会偶尔看到 both==0）。
//   seq_cst 版会插入屏障（如 mfence / xchg），使 both==0 永不发生。
//   是否复现到 both>0 取决于 CPU/调度，复现不到也属正常——
//   结论应建立在“单一全序”的推理上，而非某次运行的计数。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <functional>
#include <thread>

// =====================================================================
// litmus-test 跑法（高效版）：只起两个常驻 worker 线程，
//   用“轮次票据（ticket）”做两阶段同步，反复跑同一个 store-load 体，
//   统计“两边都读到对方旧值（0）”的次数。
//
//   为什么不每轮新建线程：创建/销毁线程极慢，会淹没真正想观测的重排，
//   且让两个线程难以“真正同时”冲进临界点。常驻线程 + 紧凑同步
//   才是 litmus test 的标准写法（参考 Preshing 的实验）。
//
//   t0_body：写 x、读 y，返回读到的 y。 t1_body：写 y、读 x，返回读到的 x。
// =====================================================================
long long run_store_load(int rounds,
                         std::function<int(std::atomic<int>&, std::atomic<int>&)> t0_body,
                         std::function<int(std::atomic<int>&, std::atomic<int>&)> t1_body) {
    std::atomic<int> x{0}, y{0};
    std::atomic<int> r0{0}, r1{0};
    std::atomic<int> turn{0};       // 当前轮次票据；worker 等它变到自己的轮号
    std::atomic<int> done0{0};      // worker0 完成的轮次数
    std::atomic<int> done1{0};      // worker1 完成的轮次数
    std::atomic<bool> stop{false};

    auto worker = [&](bool is0) {
        int my_round = 0;
        for (;;) {
            // 等主线程把 turn 推进到本轮（seq_cst 确保看到主线程对 x/y 的复位）
            while (turn.load(std::memory_order_acquire) <= my_round) {
                if (stop.load(std::memory_order_acquire)) return;
            }
            ++my_round;
            int r = is0 ? t0_body(x, y) : t1_body(x, y);
            if (is0) { r0.store(r, std::memory_order_relaxed); done0.fetch_add(1, std::memory_order_release); }
            else     { r1.store(r, std::memory_order_relaxed); done1.fetch_add(1, std::memory_order_release); }
        }
    };

    std::thread w0(worker, true);
    std::thread w1(worker, false);

    long long both_zero = 0;
    for (int i = 1; i <= rounds; ++i) {
        x.store(0, std::memory_order_relaxed);
        y.store(0, std::memory_order_relaxed);
        // 发车：把两个 worker 同时放进本轮（它们会几乎同时冲进 store-load）
        turn.store(i, std::memory_order_release);
        // 等两个 worker 都完成本轮
        while (done0.load(std::memory_order_acquire) < i ||
               done1.load(std::memory_order_acquire) < i) { /* spin */ }
        if (r0.load(std::memory_order_relaxed) == 0 &&
            r1.load(std::memory_order_relaxed) == 0) {
            ++both_zero; // 两边都没看到对方的写 => StoreLoad 重排被观测到
        }
    }

    stop.store(true, std::memory_order_release);
    turn.fetch_add(1, std::memory_order_release); // 踢醒可能在等的 worker
    w0.join();
    w1.join();
    return both_zero;
}

// =====================================================================
// 必做 1：实现 store-load 模式，并用 seq_cst 修复。
//   先用 relaxed 复现（可能 both==0），再用 seq_cst 修复（恒不为 0）。
// =====================================================================
void demo_store_load_seqcst() {
    cs::println("======== 必做 1：store-load 重排，用 seq_cst 修复 ========");

    constexpr int kRounds = 1000000;

    // ---- relaxed：可能出现 both==0（StoreLoad 重排可见） ----
    long long relaxed_both0 = run_store_load(
        kRounds,
        [](std::atomic<int>& x, std::atomic<int>& y) {
            x.store(1, std::memory_order_relaxed);            // 先写自己
            return y.load(std::memory_order_relaxed);         // 再读对方
        },
        [](std::atomic<int>& x, std::atomic<int>& y) {
            y.store(1, std::memory_order_relaxed);
            return x.load(std::memory_order_relaxed);
        });

    // ---- seq_cst：修复。所有 seq_cst 操作有单一全序，禁止 both==0 ----
    long long seqcst_both0 = run_store_load(
        kRounds,
        [](std::atomic<int>& x, std::atomic<int>& y) {
            // TODO [必做 1]: 用 seq_cst 的 store/load 修复 store-load 重排。
            //   要点：memory_order_seq_cst 让所有 seq_cst 操作落在
            //   一条全线程一致的全序上；该全序禁止“两边都读到旧值 0”。
            //   下面已是正确写法，留作必做 1 的参考实现：
            x.store(1, std::memory_order_seq_cst);
            return y.load(std::memory_order_seq_cst);
        },
        [](std::atomic<int>& x, std::atomic<int>& y) {
            y.store(1, std::memory_order_seq_cst);
            return x.load(std::memory_order_seq_cst);
        });

    cs::logf("[seqcst] 轮数=", kRounds,
             "  relaxed 出现 both==0 次数=", relaxed_both0,
             "  seq_cst 出现 both==0 次数=", seqcst_both0);
    cs::println("解读：");
    cs::println("  - relaxed 版常常 >0：x86 唯一允许的 StoreLoad 重排被观测到——"
                "‘我先写自己再读对方’被看成‘先读对方(旧值)再写自己’。");
    cs::println("  - seq_cst 版恒为 0：单一全序里，至少有一个线程的 store"
                " 排在另一个线程的 load 之前，故不可能两边都读到 0。");
    cs::println("  - 注意：acquire/release 修不了这个——它管的是 message"
                " passing 方向的可见性，不禁止 StoreLoad 重排。");
    cs::println("");
}

// =====================================================================
// 进阶 1：用 std::atomic_thread_fence(seq_cst) 替代每操作的 seq_cst。
//
//   思路：把内存序从“每个原子操作”上卸下来（store/load 用 relaxed），
//   改为在“写自己之后、读对方之前”插入一道 seq_cst 全栅栏。
//   这道全栅栏同样参与 seq_cst 全序，把本线程的 relaxed store
//   与后续 relaxed load 在全序上隔开，从而禁止 both==0。
//
//   何时偏好 fence：当你有“写一批、读一批”的关键节，与其给每个变量都贴
//   seq_cst（处处屏障），不如在关键节插一道 fence（一次屏障覆盖多个操作），
//   语义更清晰、有时也更省。
// =====================================================================
void demo_seqcst_fence() {
    cs::println("======== 进阶 1：用 atomic_thread_fence(seq_cst) 替代 ========");

    constexpr int kRounds = 1000000;

    long long fence_both0 = run_store_load(
        kRounds,
        [](std::atomic<int>& x, std::atomic<int>& y) {
            // TODO [进阶 1]: store/load 用 relaxed，中间插一道 seq_cst 全栅栏。
            //   要点：std::atomic_thread_fence(memory_order_seq_cst) 参与
            //   全局 seq_cst 全序，把“写自己”与“读对方”在全序上隔开，
            //   达到与每操作 seq_cst 相同的修复效果，却把屏障集中到一点。
            //   下面已是正确写法，留作进阶 1 的参考实现：
            x.store(1, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            return y.load(std::memory_order_relaxed);
        },
        [](std::atomic<int>& x, std::atomic<int>& y) {
            y.store(1, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            return x.load(std::memory_order_relaxed);
        });

    cs::logf("[fence] 轮数=", kRounds,
             "  seq_cst fence 版出现 both==0 次数=", fence_both0);
    cs::println("解读：relaxed store + seq_cst fence + relaxed load，"
                "两线程对称地插全栅栏，both==0 同样恒为 0——");
    cs::println("      把‘每操作 seq_cst’的开销集中成‘关键点一道 fence’。");
    cs::println("");
}

int main() {
    cs::println("==== F3_seqcst_fence：store-load 重排与 seq_cst 修复 ====\n");

    demo_store_load_seqcst(); // 必做 1：seq_cst 修复 store-load 重排
    demo_seqcst_fence();      // 进阶 1：atomic_thread_fence(seq_cst) 替代

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
