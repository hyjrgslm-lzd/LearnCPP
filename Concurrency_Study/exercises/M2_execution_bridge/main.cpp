// =====================================================================
// 练习 M-2：std::execution 桥接（sender/receiver 取代裸线程）
//   对应文档：Concurrency_Study/16-模块M-工作窃取与结构化并发桥接.md 的 练习 M-2
//
//   学习目标：
//     - 认识 C++26 的 std::execution（提案 P2300）：异步的【统一框架】——
//         * scheduler：把工作【投递到哪里执行】的抽象（线程池、GPU、单线程…）；
//         * sender：一段“【将要】产出值/错误/停止”的异步工作【描述】（惰性、可组合）；
//         * receiver：sender 完成时回调的【接收端】（值/错误/停止三条通道）；
//     - 用算法把 sender【组合】成异步管线：
//         schedule(sched) | then(f) | then(g) ...，再 sync_wait 取结果；
//         just(v) 造一个“已就绪值”的 sender；when_all 并发汇合多个 sender；
//     - 体会它【相对手写 thread + future 的好处】：
//         * 结构化（structured）：管线是一棵树，生命周期/资源随结构自然管理；
//         * 可组合（composable）：then/when_all 像搭积木，无需手摇 future.then 链；
//         * 三条通道（value/error/stopped）：错误与【取消（cancellation）】是
//           一等公民，沿管线自动传播，不像裸线程那样得自己缝异常与停止逻辑；
//         * 调度可换：把 scheduler 一换，同一段管线就能换执行资源（关注点分离）。
//
//   重要工具链说明（务必先读）：
//     - std::execution 是 C++26 标准（头文件 <execution> 的 std::execution 命名空间），
//       但截至 2026-05【MSVC 尚未实现】。本套材料用 NVIDIA 的参考实现【stdexec】回退：
//         * 命名空间是【stdexec】（不是 std::execution）；
//         * 核心算法头文件 <stdexec/execution.hpp>；
//         * 线程池等执行资源在 <exec/...>，本题用 exec::static_thread_pool；
//     - 本模块对 senders【只作桥接演示】——讲清“它怎样取代裸线程、为什么更好”。
//       sender/receiver 的完整机制（自定义 sender、域、completion signatures、
//       取消令牌细节等）【深入见 Execution_Study\】，不在本套并发材料展开。
//
//   骨架说明：关键实现处用 // TODO [必做 N]: / // TODO [进阶 N]: 标记。
//     未填 TODO 处给了参考实现（已启用）以保证可编译运行；建议遮住自己重写。
//
//   官方参考：
//     - P2300R10 std::execution：https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html
//     - NVIDIA/stdexec（参考实现 + README）：https://github.com/NVIDIA/stdexec
//     - cppreference std::execution：https://en.cppreference.com/w/cpp/execution
//
//   编译运行（VS2026, C++20；需 stdexec，CMake 已用 StdexecSetup 拉取）：
//     cmake --build build-vs2026 --target M2_execution_bridge --config Release
//     ./build-vs2026/M2_execution_bridge/Release/M2_execution_bridge.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <exception>
#include <string>
#include <tuple>
#include <utility>

// stdexec：P2300 std::execution 的 NVIDIA 参考实现（MSVC 回退）。
//   命名空间 stdexec ≈ 标准的 std::execution。
#include <stdexec/execution.hpp>
// 执行资源：静态线程池，提供一个 scheduler。
#include <exec/static_thread_pool.hpp>

int main() {
    cs::println("==== M2_execution_bridge：std::execution（P2300）sender/receiver 桥接 ====");
    cs::println("  用 stdexec（NVIDIA 参考实现，回退 MSVC 未实现的 std::execution）演示：");
    cs::println("  schedule | then 组管线、sync_wait 取结果、when_all 并发汇合。");
    cs::println("  深入机制见 Execution_Study\\。\n");

    // -----------------------------------------------------------------
    // 准备执行资源：一个静态线程池，从中拿到一个 scheduler。
    //   scheduler 回答的是“工作【在哪执行】”——这里是线程池的某个线程。
    // -----------------------------------------------------------------
    exec::static_thread_pool pool(4);
    stdexec::scheduler auto sched = pool.get_scheduler();

    // =================================================================
    // 必做 1：schedule | then | then 组成异步管线，sync_wait 取结果。
    //   对照手写版：你本来要 std::thread 起线程、std::promise/future 回传、
    //   手动 join、手动 try/catch 包异常……这里一条管线全包了。
    // =================================================================
    cs::println("======== 必做 1：schedule | then | then 管线 + sync_wait ========");
    {
        // TODO [必做 1]: 用 schedule(sched) 起头，接两个 then 变换，sync_wait 取值。
        //   要点：
        //     - stdexec::schedule(sched) 产出一个“在 sched 上开始、不带值”的 sender；
        //     - then(f) 把上游结果喂给 f，f 的返回值成为新 sender 的值（像 future.then）；
        //     - 也可用 just(v) 造一个“已就绪值 v”的 sender 作为起点；
        //     - sync_wait(s) 在【当前线程】阻塞等到管线完成，返回 std::optional<std::tuple<...>>
        //       （值在 tuple 里）。这是“异步世界”与“同步 main”之间的桥。
        //   参考实现（已启用以保证可运行；建议遮住自己重写）：
        auto pipeline =
            stdexec::schedule(sched)
            | stdexec::then([] {
                  cs::logf("[then-1] 在池线程上开始，产出 21");
                  return 21;
              })
            | stdexec::then([](int x) {
                  cs::logf("[then-2] 收到 ", x, "，翻倍");
                  return x * 2;
              });

        auto result = stdexec::sync_wait(std::move(pipeline)); // 阻塞直到完成
        const int got = std::get<0>(result.value());
        cs::logf("[main] 管线结果 = ", got, "，期望 42 -- ",
                 (got == 42 ? "一致 OK" : "不一致 MISMATCH"));
    }
    cs::println("");

    // =================================================================
    // 必做 2：when_all 并发两个 sender，汇合后一起取结果。
    //   对照手写版：你本来要起两个线程、两个 future，再各 get()/join()；
    //   when_all 把“等全部完成”这件事变成一个【可组合的 sender】。
    // =================================================================
    cs::println("======== 必做 2：when_all 并发两个 sender ========");
    {
        // TODO [必做 2]: 用 when_all 并发跑两条管线，再 then 把两个结果合起来。
        //   要点：
        //     - 两个上游 sender 各自在线程池上跑（并发）；
        //     - when_all(a, b) 产出“两个值”的 sender；下游 then 接 (va, vb)；
        //     - 任一上游报错/被取消，整体沿 error/stopped 通道传播（无需你手缝）。
        //   参考实现（已启用以保证可运行）：
        auto a = stdexec::schedule(sched)
               | stdexec::then([] { cs::logf("[a] 计算 100"); return 100; });
        auto b = stdexec::schedule(sched)
               | stdexec::then([] { cs::logf("[b] 计算 23"); return 23; });

        auto both =
            stdexec::when_all(std::move(a), std::move(b))
            | stdexec::then([](int va, int vb) {
                  cs::logf("[join] 汇合 ", va, " + ", vb);
                  return va + vb;
              });

        auto result = stdexec::sync_wait(std::move(both));
        const int got = std::get<0>(result.value());
        cs::logf("[main] when_all 结果 = ", got, "，期望 123 -- ",
                 (got == 123 ? "一致 OK" : "不一致 MISMATCH"));
    }
    cs::println("");

    // =================================================================
    // 进阶：just(...) 起头 + 错误通道演示（异常如何沿管线传播）。
    // =================================================================
    cs::println("======== 进阶：just 起头 + 错误通道（error channel）========");
    {
        // TODO [进阶 1]: 用 just(v) 造起始 sender，链 then；再演示 then 内抛异常
        //   如何走【error 通道】被 sync_wait 重新抛出（结构化错误传播）。
        //   要点：sync_wait 会把管线 error 通道里的异常在调用线程【重新抛出】，
        //   你只需正常 try/catch，无需像裸线程那样手动跨线程搬运异常。
        //   参考实现（已启用以保证可运行）：
        auto ok = stdexec::just(10)
                | stdexec::then([](int x) { return x + 5; }); // 15
        auto r1 = stdexec::sync_wait(std::move(ok));
        cs::logf("[main] just(10)|then(+5) = ", std::get<0>(r1.value()),
                 "，期望 15 -- ", (std::get<0>(r1.value()) == 15 ? "OK" : "MISMATCH"));

        auto boom = stdexec::just(1)
                  | stdexec::then([](int) -> int {
                        throw std::runtime_error("boom in pipeline");
                    });
        try {
            stdexec::sync_wait(std::move(boom));
            cs::logf("[main] 异常未传播 -- MISMATCH");
        } catch (const std::exception& e) {
            cs::logf("[main] sync_wait 经 error 通道重抛异常：", e.what(), " -- OK");
        }
    }
    cs::println("");

    cs::println("为什么 sender/receiver 比裸 thread+future 好（本题要点）：");
    cs::println("  - 结构化：管线是一棵树，资源/生命周期随结构管理，无裸 join 散落各处；");
    cs::println("  - 可组合：then/when_all 像搭积木，免手摇 future.then 链与回调地狱；");
    cs::println("  - 三通道：value/error/stopped 一等公民，错误与取消沿管线自动传播；");
    cs::println("  - 调度可换：换个 scheduler 即换执行资源（线程池/GPU），管线不动。");
    cs::println("  - 深入（自定义 sender、completion signatures、取消令牌）见 Execution_Study\\。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
