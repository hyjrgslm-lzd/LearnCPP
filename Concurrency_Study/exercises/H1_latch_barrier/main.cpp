// =====================================================================
// 练习 H-1：latch 与 barrier（一次性栅栏 vs 可重用屏障）
//   对应文档：Concurrency_Study/10-模块H-高级同步原语.md 的 练习 H-1
//
//   学习目标：
//     - 掌握 std::latch（闩，<latch>，C++20）：一次性向下计数的栅栏。
//       N 个 worker 各 count_down() 一次，主线程 wait() 到计数归零后放行。
//       计数到 0 后【不可重用】（不能重置回 N）；
//     - 掌握 std::barrier（屏障，<barrier>，C++20）：可【重用】的多阶段汇合点。
//       每个 worker 每阶段 arrive_and_wait()，到齐后自动进入下一阶段；
//       构造时给的 completion function（完成函数）在【每个阶段末】由
//       某一个（实现选定的）线程执行【恰好一次】，常用于阶段收尾/汇总；
//     - 说清两者的本质区别：latch 计数一次性、不绑定参与线程数语义之外的状态；
//       barrier 维护“代（generation / phase）”，到齐即自动重置、可循环多轮；
//     - 体会“全部就绪后同时开跑”（latch）与“多轮迭代的相位同步”（barrier）
//       这两类经典并发编排模式。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/latch
//     - https://en.cppreference.com/w/cpp/thread/barrier
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章
//       （4.4.x latch 与 barrier 一节）
//     - 提案 P1135R6（The C++20 Synchronization Library）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target H1_latch_barrier --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <barrier>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

// =====================================================================
// 第一部分：std::latch —— “全部就绪后同时开跑”（一次性栅栏）
//
//   场景：起跑线模型。主线程造好 N 个 worker，但希望它们【同时】开跑，
//   而不是谁先 ready 谁先跑。做法：
//     - 一个 latch ready(N)：每个 worker 完成准备工作后 count_down() 一次；
//     - 主线程 ready.wait()：阻塞到计数归零（N 个都准备好），再宣布“开跑”；
//     - 同时用另一个 latch go(1) 当“发令枪”：worker 准备完后 go.wait() 等
//       发令枪；主线程见全部就绪后 go.count_down() 放行——于是大家几乎同时起跑。
//
//   关键性质：latch 一旦计数到 0 就【永久】放行，无法重置回 N（不可重用）。
//             需要重用请改用 barrier（见第二部分）。
// =====================================================================
void demo_latch() {
    cs::println("\n---- 第一部分：std::latch（一次性，全部就绪后同时开跑）----");

    constexpr int kWorkers = 4;

    // ready：N 个 worker 各 count_down 一次，主线程等它归零（确认全部就绪）。
    std::latch ready(kWorkers);
    // go：发令枪。主线程见全部就绪后 count_down(1) 放行，worker 在此 wait。
    std::latch go(1);

    std::atomic<int> started_count{0};
    std::vector<std::thread> workers;

    for (int i = 0; i < kWorkers; ++i) {
        workers.emplace_back([i, &ready, &go, &started_count] {
            // 模拟“准备阶段”：各 worker 准备耗时不同（故意错开）。
            std::this_thread::sleep_for(std::chrono::milliseconds(20 * (i + 1)));
            cs::logf("[worker ", i, "] 准备就绪。");

            // TODO [必做 1]: 用 latch 实现“全部就绪后同时开跑”。
            //   两步：
            //     (a) 报告自己已就绪：对 ready 调用 count_down()（计数 -1）；
            //     (b) 在发令枪前等待：对 go 调用 wait()，阻塞到主线程放行。
            //   完成后请遮住下面参考实现重写一遍。
            //
            //   参考实现（已启用以保证可编译运行）：
            ready.count_down();   // (a) 我准备好了，计数 -1。
            go.wait();            // (b) 等发令枪（go 计数归零）。

            // —— 发令枪响后，这里几乎同时被唤醒 ——
            started_count.fetch_add(1, std::memory_order_relaxed);
            cs::logf("[worker ", i, "] 开跑！");
        });
    }

    cs::logf("[main] 等待全部 ", kWorkers, " 个 worker 就绪……");
    ready.wait(); // 阻塞到 ready 计数归零：N 个 worker 全部 count_down 完毕。
    cs::logf("[main] 全部就绪。鸣枪！（go.count_down -> 同时放行）");
    go.count_down(); // 发令枪：go 计数 1->0，所有 go.wait() 的 worker 被唤醒。

    for (auto& th : workers) th.join();
    cs::logf("[main] 本轮起跑的 worker 数 = ", started_count.load(),
             "（应为 ", kWorkers, "）。");
    cs::println("  注意：latch 计数到 0 后【不可重用】——要多轮请用 barrier。");
}

// =====================================================================
// 第二部分：std::barrier —— 多轮迭代的相位同步 + completion 收尾
//
//   场景：迭代式并行计算（如分阶段流水：每轮所有 worker 各算一块，
//   全部到齐后由“某一个线程”做一次本轮汇总/收尾，再一起进入下一轮）。
//
//   barrier 的三个要点：
//     1) 构造 barrier(N, completion)：N 是每阶段需要到齐的参与者数；
//        completion 是“完成函数”，在【每个阶段所有参与者都 arrive 后】，
//        由实现选定的某【一个】线程执行【恰好一次】，然后才解除所有线程的阻塞。
//     2) arrive_and_wait()：本线程“到达 + 等待”。等价于 wait(arrive())。
//        当本阶段最后一个参与者 arrive 时，触发 completion，再唤醒全部。
//     3) 可【重用】：每阶段结束后 barrier 自动重置回 N，进入下一“代”，
//        于是可以在循环里反复用同一个 barrier 做多轮相位同步。
//
//   completion function 的签名要求：可被无参调用、且 noexcept（不抛异常）。
// =====================================================================
void demo_barrier() {
    cs::println("\n---- 第二部分：std::barrier（可重用，多轮相位同步 + completion）----");

    constexpr int kWorkers = 3;
    constexpr int kRounds = 4;

    // 每轮各 worker 把自己算出的“贡献”累加到 round_sum；completion 在每轮末
    // 打印汇总并清零，为下一轮做准备。completion 与 worker 之间天然无数据竞争：
    // completion 在“全部 arrive 之后、唤醒之前”单线程执行（happens-before 下一轮）。
    std::atomic<int> round_sum{0};
    std::atomic<int> round_index{0};

    // completion function：每个阶段末由某一个线程执行恰好一次。必须 noexcept。
    auto on_phase_done = [&round_sum, &round_index]() noexcept {
        const int idx = round_index.fetch_add(1, std::memory_order_relaxed);
        const int sum = round_sum.exchange(0, std::memory_order_relaxed);
        // 此处由“某一个”线程单独执行；其余线程都还阻塞在 barrier 上。
        cs::logf("[completion] 第 ", idx, " 轮汇总：本轮 round_sum = ", sum,
                 "（已清零，准备下一轮）。");
    };

    // 构造可重用 barrier：N=kWorkers，附带 completion function。
    std::barrier sync_point(kWorkers, on_phase_done);

    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkers; ++i) {
        workers.emplace_back([i, kRounds, &sync_point, &round_sum] {
            for (int r = 0; r < kRounds; ++r) {
                // 模拟本轮该 worker 的计算：贡献一个值。
                const int contribution = (r + 1) * 10 + i; // 随轮次/编号变化
                round_sum.fetch_add(contribution, std::memory_order_relaxed);
                cs::logf("[worker ", i, "] 第 ", r, " 轮计算完成，贡献 ", contribution, "。");

                // TODO [必做 2]: 用 barrier 做多轮相位同步。
                //   调用 arrive_and_wait()：本线程到达本轮屏障并等待。
                //   当本轮最后一个 worker 到达时，barrier 触发 completion
                //   （打印汇总并清零），随后唤醒全部 worker 进入下一轮。
                //   barrier 可重用：循环里反复用同一个 sync_point。
                //
                //   参考实现（已启用以保证可编译运行）：
                sync_point.arrive_and_wait();

                // —— 屏障放行后，所有 worker 同步进入下一轮 ——
            }
            cs::logf("[worker ", i, "] 全部 ", kRounds, " 轮完成，退出。");
        });
    }

    for (auto& th : workers) th.join();
    cs::logf("[main] barrier 多轮演示结束，共 ", round_index.load(), " 轮触发 completion。");

    // 进阶提示（见文档“进阶任务”）：
    //   - arrive_and_drop()：让本线程在“到达本阶段后”永久退出参与，后续阶段
    //     的到齐人数自动 -1。可用于“某 worker 提前完工但不拖累其余线程继续多轮”。
    //   - arrive() 返回 arrival_token，可与 wait(token) 拆开，先做点别的再等。
    cs::println("  注意：barrier 每阶段自动重置、可重用；completion 每阶段只执行一次。");
}

int main() {
    cs::println("==== H1_latch_barrier：latch（一次性）vs barrier（可重用）====");

    demo_latch();
    demo_barrier();

    cs::println("\n小结：");
    cs::println("  - latch：一次性向下计数栅栏，归零后放行且不可重用；");
    cs::println("           适合“等 N 件事都完成 / 全部就绪后同时开跑”。");
    cs::println("  - barrier：可重用的多阶段汇合点，每阶段到齐触发 completion 一次，");
    cs::println("             随后自动进入下一代；适合多轮迭代的相位同步。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
