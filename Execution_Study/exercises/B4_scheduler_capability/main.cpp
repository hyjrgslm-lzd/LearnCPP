#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>

namespace ex = stdexec;

// ── 辅助：线程安全打印 ───────────────────────────────
void log_task(int task_id) {
    std::ostringstream oss;
    oss << "  [task " << task_id << "] thread_id = "
        << std::this_thread::get_id() << "\n";
    std::cout << oss.str();
}

// ══════════════════════════════════════════════════════
// TODO [必做]: 实现 run_with_pool
//   1. 创建 exec::static_thread_pool(thread_count)
//   2. 获取 scheduler: auto sch = pool.get_scheduler();
//   3. 构造 8 个独立 sender，每个从 ex::schedule(sch) 开始，
//      在 then 中调用 log_task(task_id) 打印任务编号和线程 ID
//   4. 用 ex::when_all(...) 汇合全部 sender
//   5. 用 ex::sync_wait(...) 等待完成
// ══════════════════════════════════════════════════════
void run_with_pool(int thread_count) {
    std::cout << "\n=== static_thread_pool(" << thread_count << ") ===\n";

    // TODO [必做]: 创建线程池
    // exec::static_thread_pool pool(thread_count);
    // auto sch = pool.get_scheduler();

    // TODO [必做]: 构造 8 个 sender 分支
    //   提示：可以用辅助 lambda 批量创建
    //
    // auto make_task = [&](int task_id) {
    //     return ex::schedule(sch)
    //         | ex::then([task_id]() {
    //             log_task(task_id);
    //         });
    // };
    //
    // auto all = ex::when_all(
    //     make_task(0), make_task(1), make_task(2), make_task(3),
    //     make_task(4), make_task(5), make_task(6), make_task(7)
    // );

    // TODO [必做]: 用 sync_wait 消费
    // ex::sync_wait(std::move(all));

    std::cout << "=== done ===\n";
}

int main() {
    std::cout << "主线程 thread_id = " << std::this_thread::get_id() << "\n";

    // 先用 1 个工作线程跑一遍
    run_with_pool(1);

    // 再用 4 个工作线程跑一遍
    run_with_pool(4);

    // ══════════════════════════════════════════════════════
    // TODO [进阶]: 在每个任务里加入极短的 sleep_for，
    //   让线程分布更容易观察。
    // TODO [进阶]: 用同一个 scheduler 连续创建两批任务，
    //   观察 scheduler 可复制、可重复使用这一点。
    // ══════════════════════════════════════════════════════

    return 0;
}
