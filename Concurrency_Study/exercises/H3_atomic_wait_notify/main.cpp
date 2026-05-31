// =====================================================================
// 练习 H-3：atomic wait / notify（原子等待与通知，类 futex）
//   对应文档：Concurrency_Study/10-模块H-高级同步原语.md 的 练习 H-3
//
//   学习目标：
//     - 掌握 std::atomic<T>::wait(old) / notify_one() / notify_all()（C++20）：
//       wait(old) —— 若当前原子值【等于】old，则【阻塞】本线程，直到被通知
//       且值发生改变（再加载比较）；若已不等于 old，立即返回（不阻塞）。
//       notify_one()/notify_all() —— 唤醒一个 / 全部正在 wait 的线程；
//     - 理解它相对【忙等自旋（busy-wait / spin）】的优势：自旋会持续占满 CPU、
//       浪费功耗、与其他线程抢核；wait 会让线程真正【睡眠】，由内核/运行时
//       择机唤醒（平台上常用 futex / WaitOnAddress 之类的“按地址等待”原语实现），
//       几乎不烧 CPU；
//     - 用 atomic flag + wait/notify 实现一个高效的“一次性事件 / flag”，
//       并与朴素 spin 轮询对比；
//     - 记住核心使用范式：
//         等待方：while (flag.load(mo) == old) flag.wait(old, mo);
//         通知方：flag.store(new, mo); flag.notify_one()/notify_all();
//       （wait 可能伪唤醒/ABA 唤醒，所以一定要在循环里复查谓词。）
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic/wait
//     - https://en.cppreference.com/w/cpp/atomic/atomic/notify_one
//     - https://en.cppreference.com/w/cpp/atomic/atomic/notify_all
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 / 5 章
//     - 提案 P1135R6（The C++20 Synchronization Library）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target H3_atomic_wait_notify --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <thread>

// =====================================================================
// 第一部分：用 atomic flag + wait/notify 实现高效的一次性事件
//
//   场景：一个 waiter 线程要等 signaler 线程“拉起事件”后才继续。
//
//   朴素做法（反面教材，见第二部分）：waiter 在一个 while 里反复 load 自旋，
//   不停轮询——CPU 被烧满，事件还没来时纯属空转。
//
//   高效做法（本部分）：
//     等待方：while (flag.load(acquire) == 0) flag.wait(0, acquire);
//       —— 只要还是 0 就 wait(0)：内核把本线程挂起，几乎不耗 CPU。
//     通知方：flag.store(1, release); flag.notify_one();
//       —— 改值并唤醒等待者。release/acquire 还顺带把 signaler 写好的数据
//          发布给 waiter（见模块 F），所以 waiter 醒来能看到完整数据。
//
//   为什么 while 而不是 if：wait 允许“伪唤醒（spurious wakeup）”以及
//   “值变了又变回来”等情形——醒来后必须复查谓词，不满足就接着 wait。
// =====================================================================
void demo_atomic_wait_notify() {
    cs::println("\n---- 第一部分：atomic flag + wait/notify（高效等待）----");

    std::atomic<int> flag{0};      // 0 = 事件未发生，1 = 已发生（一次性）
    std::atomic<int> payload{0};   // 事件携带的数据

    std::thread waiter([&] {
        cs::logf("[waiter] 进入等待（atomic::wait，睡眠而非自旋，几乎不烧 CPU）……");

        // TODO [必做 1]: 用 atomic flag + wait/notify 实现高效等待。
        //   在 while 谓词循环里调用 flag.wait(0, acquire)：
        //   只要 flag 仍为 0，就阻塞本线程；被通知且值变化后醒来复查。
        //   完成后请遮住下面参考实现重写一遍。
        //
        //   参考实现（已启用以保证可编译运行）：
        while (flag.load(std::memory_order_acquire) == 0) {
            // wait(old)：若当前值 == old(0) 则阻塞；被 notify 且值变化后返回。
            flag.wait(0, std::memory_order_acquire);
            // 醒来后回到 while 复查谓词（应对伪唤醒）。
        }

        // 与 signaler 的 release 配对：此时一定能看到它写好的 payload。
        cs::logf("[waiter] 被唤醒，读到 payload = ",
                 payload.load(std::memory_order_relaxed), "。");
    });

    std::thread signaler([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(80)); // 模拟准备
        payload.store(7, std::memory_order_relaxed);                // 先写数据

        // TODO [必做 2]: 通知等待方。
        //   先 store 改变 flag（release，发布 payload），再 notify_one() 唤醒 waiter。
        //   顺序铁律：先改值、再 notify——否则 waiter 复查谓词时可能错过唤醒。
        //
        //   参考实现（已启用以保证可编译运行）：
        cs::logf("[signaler] 事件就绪，flag.store(1, release) 并 notify_one()。");
        flag.store(1, std::memory_order_release); // 改值（顺带发布 payload）
        flag.notify_one();                        // 唤醒一个 wait 的线程
    });

    waiter.join();
    signaler.join();
}

// =====================================================================
// 第二部分：对比朴素忙等自旋（busy-wait / spin）—— 反面教材
//
//   同样等一个 flag，但用 while 空转轮询：CPU 被持续占满，事件没来时纯空耗。
//   这里加一个自旋计数器，把“事件来临前白白转了多少圈”打出来——直观感受
//   spin 的浪费。短临界/极短等待时 spin 可能更快（省一次睡眠/唤醒），
//   但等待时间不可忽略时，wait/notify 才是正解。
//
//   （本函数仅为对比演示，故意保留低效写法；生产代码请用第一部分的 wait/notify。）
// =====================================================================
void demo_busy_spin_for_contrast() {
    cs::println("\n---- 第二部分：朴素忙等自旋（对比，反面教材）----");

    std::atomic<int> flag{0};
    std::atomic<long long> spin_count{0};

    std::thread spinner([&] {
        long long local_spins = 0;
        // 朴素自旋：只要还是 0 就空转复查——CPU 被这条线程吃满。
        while (flag.load(std::memory_order_acquire) == 0) {
            ++local_spins;
            // 退一步：yield 让出时间片，稍微缓解占用（但仍是忙等，不会真正睡眠）。
            std::this_thread::yield();
        }
        spin_count.store(local_spins, std::memory_order_relaxed);
        cs::logf("[spinner] 事件到来前，白白自旋了约 ", local_spins,
                 " 圈（CPU 被持续占用）。");
    });

    std::thread setter([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        cs::logf("[setter] 拉起事件 flag.store(1)。");
        flag.store(1, std::memory_order_release);
        // 注意：朴素自旋方根本不 wait，所以这里【无需】notify——它靠空转复查发现。
    });

    spinner.join();
    setter.join();

    cs::println("  对比：spin 在等待期间空转上述圈数、烧满 CPU；");
    cs::println("        atomic::wait 让线程睡眠（平台常用 futex/WaitOnAddress），几乎 0 占用。");
}

int main() {
    cs::println("==== H3_atomic_wait_notify：原子等待/通知 vs 忙等自旋 ====");

    demo_atomic_wait_notify();
    demo_busy_spin_for_contrast();

    cs::println("\n小结：");
    cs::println("  - atomic<T>::wait(old)：值==old 时阻塞（睡眠），被 notify 且值变化后返回；");
    cs::println("    notify_one/notify_all 唤醒等待者。范式：while(load==old) wait(old)。");
    cs::println("  - 相对 spin 的优势：不烧 CPU、不抢核、省功耗；平台常以 futex 类原语实现。");
    cs::println("  - 顺序铁律：通知方【先改值、再 notify】；等待方在 while 谓词里复查（防伪唤醒）。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
