// =====================================================================
// 练习 D-2：std::async 启动策略（launch::async / deferred / 默认）
//   对应文档：Concurrency_Study/05-模块D-future与异步任务.md 的 练习 D-2
//
//   学习目标：
//     - 对比 std::async 三种启动策略在“执行线程”和“执行时机”上的差异：
//         launch::async    保证新线程、立即开跑；
//         launch::deferred 惰性、get() 时才在调用线程同步执行；
//         默认(async|deferred) 实现自选，不可假设异步；
//     - 演示并解释著名陷阱：std::async 返回的 future 析构会阻塞，
//       不保存返回值（临时 future）会让多个 async 调用退化成串行；
//     - 用 future_status::deferred 观测 deferred 的惰性。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/async
//     - https://en.cppreference.com/w/cpp/thread/launch
//     - https://en.cppreference.com/w/cpp/thread/future/wait_for
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章 4.2.1
//     - Scott Meyers《Effective Modern C++》Item 35–36
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target D2_async_policies --config Release
//     ./build-vs2026/D2_async_policies/Release/D2_async_policies.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// 一个会打印自己所在线程 id 的任务：用来肉眼区分“在哪个线程跑”。
int announce_task(int id) {
    cs::logf("  >> task ", id, " 正在线程 ", std::this_thread::get_id(), " 上执行。");
    return id * 10;
}

// 一个固定耗时的“慢任务”：用于计时对比串行 vs 并行。
int slow_task(int id) {
    std::this_thread::sleep_for(200ms);
    return id;
}

// =====================================================================
// 必做 1：三种启动策略对比（线程 + 时机两个维度）。
// =====================================================================
void demo_policies() {
    cs::println("============ 必做 1：launch::async / deferred / 默认 ============");
    cs::logf("  主线程 id = ", std::this_thread::get_id());

    // TODO [必做 1]: 分别用三种策略各发起一次 async，观察差异。
    //   下面已是正确写法，留作必做 1 的参考实现。

    // --- launch::async：保证新线程、立即开跑（get 之前就已经在跑） ---
    cs::println("--- (a) launch::async：新线程 + 立即开跑 ---");
    {
        std::future<int> fa = std::async(std::launch::async, announce_task, 1);
        // 先睡一会，让 async 任务有机会在“别的线程”上先打印出来，
        // 证明它不等我们 get() 就已经开跑。
        std::this_thread::sleep_for(100ms);
        cs::logf("  (a) 主线程随后才 get()，结果 = ", fa.get());
    }

    // --- launch::deferred：惰性；get() 时才在“调用线程”同步执行 ---
    cs::println("--- (b) launch::deferred：惰性 + 调用线程同步执行 ---");
    {
        std::future<int> fd = std::async(std::launch::deferred, announce_task, 2);
        // 此刻任务一动不动：wait_for(0) 返回 deferred。
        auto st = fd.wait_for(0s);
        cs::logf("  (b) get() 之前 wait_for(0s) == deferred? ",
                 (st == std::future_status::deferred));
        std::this_thread::sleep_for(100ms);
        cs::logf("  (b) 主线程现在 get()，注意下一行 task 的线程 id == 主线程：");
        cs::logf("  (b) 结果 = ", fd.get()); // 触发执行：在本线程同步跑
    }

    // --- 默认策略（async | deferred）：实现自选，不可假设异步 ---
    cs::println("--- (c) 默认策略：实现自选 async 或 deferred ---");
    {
        std::future<int> fdef = std::async(announce_task, 3); // 不传策略
        auto st = fdef.wait_for(0s);
        const char* kind =
            (st == std::future_status::deferred) ? "deferred（被推迟，get 时才跑）"
                                                 : "已在异步执行或已就绪（async）";
        cs::logf("  (c) 本次实现实际选择：", kind);
        cs::logf("  (c) 结果 = ", fdef.get());
    }
    cs::println("");
}

// =====================================================================
// 必做 2：future 析构阻塞 → 串行化陷阱（反面）与保存 future（正面）。
// =====================================================================
void demo_destructor_blocks() {
    cs::println("===== 必做 2：future 析构阻塞 → 串行 vs 保存 future → 并行 =====");
    constexpr int kN = 4;

    // --- 反面：不保存返回的 future ---
    // 每条 std::async(launch::async, slow_task, i) 的返回值是个临时 future，
    // 在该语句分号处立即析构；而与 async 策略关联的 future 析构会阻塞到
    // 任务跑完。于是 4 个任务一个接一个串行执行：总耗时 ≈ 4 × 200ms。
    cs::println("--- (反面) 不保存 future：临时 future 当场析构阻塞 → 串行 ---");
    {
        auto t0 = std::chrono::steady_clock::now();

        // TODO [必做 2]: 这是反面教材——故意不保存返回值。
        //   每行末尾分号 = 临时 future 析构点 = 阻塞点。
        //   下面保留这一“错误”写法用于演示串行化。
        //   注：MSVC 会就此发出 C4858（“async 返回值应被保存”）——这正是
        //   本演示要展示的坑；此处局部禁用该告警以保持构建输出干净。
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4858) // 故意丢弃 std::async 返回值（演示串行化陷阱）
#endif
        for (int i = 0; i < kN; ++i) {
            std::async(std::launch::async, slow_task, i); // 临时 future 立即析构 → 阻塞
        }
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        cs::logf("  (反面) ", kN, " 个任务总耗时 ≈ ", ms,
                 " ms（≈ N×200ms，串行！根因：async future 析构阻塞）");
    }

    // --- 正面：把 future 存进 vector，先全部发起，再统一 get ---
    // future 们存活到 vector 销毁/被 get 之前，4 个任务因此真正同时在跑：
    // 总耗时 ≈ 200ms（并行）。
    cs::println("--- (正面) 保存 future 到 vector：先全发起，后统一 get → 并行 ---");
    {
        auto t0 = std::chrono::steady_clock::now();

        std::vector<std::future<int>> futs;
        futs.reserve(kN);
        for (int i = 0; i < kN; ++i) {
            // 存住每个 future，析构被推迟，任务得以并行。
            futs.push_back(std::async(std::launch::async, slow_task, i));
        }
        for (auto& f : futs) (void)f.get(); // 统一收割

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        cs::logf("  (正面) ", kN, " 个任务总耗时 ≈ ", ms,
                 " ms（≈ 200ms，并行！因为 future 被存住、析构推迟到一起）");
    }
    cs::println("");
}

// =====================================================================
// 进阶 1：deferred 的惰性可观测 + async 传异常。
// =====================================================================
void demo_deferred_lazy_and_exception() {
    cs::println("======== 进阶 1：deferred 惰性可观测 + async 传异常 ========");

    // --- deferred 真的没跑：多次 wait_for 始终是 deferred，get() 才触发 ---
    {
        std::future<int> fd = std::async(std::launch::deferred, announce_task, 9);
        cs::logf("  (i) 第一次 wait_for(0s) deferred? ",
                 (fd.wait_for(0s) == std::future_status::deferred));
        std::this_thread::sleep_for(150ms);
        cs::logf("  (i) 睡 150ms 后再查仍 deferred? ",
                 (fd.wait_for(0s) == std::future_status::deferred),
                 "（证明它真没在后台跑）");
        cs::logf("  (i) 现在 get() 触发执行（下一行 task 线程 id == 主线程）：");
        cs::logf("  (i) 结果 = ", fd.get());
    }

    // --- async 内部也是用 promise 实现：任务抛异常，get() 处重新抛出 ---
    {
        std::future<int> fe = std::async(std::launch::async, [] () -> int {
            throw std::runtime_error("async 任务内抛出：kaboom");
        });
        try {
            (void)fe.get();
            cs::logf("  (ii) 不应到达此处");
        } catch (const std::exception& e) {
            cs::logf("  (ii) async future.get() 重新抛出异常：", e.what());
        }
    }
    cs::println("");
}

int main() {
    cs::println("==== D2_async_policies：std::async 启动策略与析构阻塞陷阱 ====\n");

    demo_policies();                      // 必做 1：三种策略对比
    demo_destructor_blocks();             // 必做 2：析构阻塞 → 串行 vs 并行
    demo_deferred_lazy_and_exception();   // 进阶 1：deferred 惰性 + 传异常

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
