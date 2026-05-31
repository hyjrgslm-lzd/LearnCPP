// =====================================================================
// 练习 D-3：packaged_task 打包任务（std::packaged_task）
//   对应文档：Concurrency_Study/05-模块D-future与异步任务.md 的 练习 D-3
//
//   学习目标：
//     - 用 std::packaged_task 把可调用对象打包成“可投递、执行后自动
//       把结果/异常写进 future”的任务单元，用 get_future() 取出 future；
//     - 把若干 packaged_task 放进任务队列，由 worker 线程取出执行，
//       主线程统一收集 futures（含异常路径）；
//     - 理清 promise / packaged_task / async 三者递进分工；
//     - 为 Capstone1 线程池埋下“任务 = packaged_task”+ submit 返回
//       future 的接口直觉（类型擦除统一成 packaged_task<void()>）。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/packaged_task
//     - https://en.cppreference.com/w/cpp/thread/packaged_task/get_future
//     - https://en.cppreference.com/w/cpp/thread/packaged_task/operator()
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章 4.2.2；第 9 章
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target D3_packaged_task --config Release
//     ./build-vs2026/D3_packaged_task/Release/D3_packaged_task.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;

// =====================================================================
// 必做 1：打包一个可调用对象、取其 future、在另一个线程上调用它。
//   观察：你从没手动 set_value，结果是 task 被调用（task()）时自动写入的。
// =====================================================================
void demo_basic_packaged_task() {
    cs::println("======== 必做 1：打包可调用对象 + 跨线程调用 ========");

    // TODO [必做 1]: 创建 packaged_task<int()>、先取 future、再 move 到线程调用。
    //   要点：get_future() 必须在 task 被 move 走之前调用并保存；
    //   task() 被调用时，返回值自动写入共享状态，future 随即就绪。
    //   下面已是正确写法，留作必做 1 的参考实现：
    std::packaged_task<int()> task([] {
        cs::logf("  [basic] 任务正在线程 ", std::this_thread::get_id(), " 上执行。");
        return 42;
    });
    std::future<int> fut = task.get_future(); // 先取 future（task 即将离手）

    std::thread t(std::move(task)); // packaged_task 本身可作为线程入口被调用
    cs::logf("  [basic] 主线程 future.get() 等待（无需手动 set_value）…");
    cs::logf("  [basic] 结果 = ", fut.get());
    t.join();
    cs::println("");
}

// =====================================================================
// 一个最小的线程安全任务队列：装类型擦除后的 packaged_task<void()>。
//   套用模块 C 的“mutex + condition_variable + 谓词 wait + closed_ 标志”。
// =====================================================================
class TaskQueue {
public:
    // 投递一个任务（move 进队列）。
    void push(std::packaged_task<void()> t) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push(std::move(t));
        }
        cv_.notify_one();
    }

    // 取出一个任务；若队列已关闭且排空，返回 false（让 worker 退出）。
    bool pop(std::packaged_task<void()>& out) {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) return false; // closed_ 且排空 → 退出信号
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    // 关闭：不再接受新任务；唤醒所有 worker 去重新检查谓词（广播型事件）。
    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::packaged_task<void()>> q_;
    bool closed_ = false;
};

// worker 循环：取任务 → 调用 → 直到队列关闭且排空。
void worker_loop(TaskQueue& tq, int worker_id) {
    cs::logf("  [queue] worker ", worker_id, " (tid ",
             std::this_thread::get_id(), ") 启动。");
    std::packaged_task<void()> task;
    while (tq.pop(task)) {
        task(); // 调用任务：结果/异常自动写入它各自的 future
    }
    cs::logf("  [queue] worker ", worker_id, " 退出（队列已关闭且排空）。");
}

// =====================================================================
// 必做 2 + 必做 3：任务队列 + worker 线程；含异常任务。
//   主线程对每个任务先 get_future() 存起来，再 move 进队列；
//   worker 执行后主线程逐个 get()（其中一个会重新抛出异常）。
// =====================================================================
void demo_task_queue() {
    cs::println("==== 必做 2/3：packaged_task 任务队列 + worker（含异常）====");

    TaskQueue tq;
    constexpr int kWorkers = 2;
    std::vector<std::thread> workers;
    for (int w = 0; w < kWorkers; ++w)
        workers.emplace_back(worker_loop, std::ref(tq), w);

    constexpr int kTasks = 6;
    std::vector<std::future<int>> futs;
    futs.reserve(kTasks);

    // TODO [必做 2]: 造 N 个 packaged_task<int()>；
    //   对每个【先 get_future() 存 vector】，再 move 进队列。
    // TODO [必做 3]: 让其中一个任务抛异常，验证它经同一条共享状态通道传回。
    //   下面已是正确写法，留作必做 2/3 的参考实现：
    for (int i = 0; i < kTasks; ++i) {
        std::packaged_task<int()> task([i] {
            if (i == 3) // 第 3 个任务故意失败，演示异常路径
                throw std::runtime_error("task 3 失败：deliberate boom");
            std::this_thread::sleep_for(20ms);
            return i * 100;
        });
        futs.push_back(task.get_future());        // 先存 future（投递后就够不着 task 了）
        // 类型擦除：把 packaged_task<int()> 包进 packaged_task<void()> 统一入队。
        tq.push(std::packaged_task<void()>(
            [t = std::move(task)]() mutable { t(); }));
    }

    // 所有任务已投递；关闭队列让 worker 排空后退出。
    tq.close();

    // 收割：逐个 get()，其中第 3 个会重新抛出异常。
    for (int i = 0; i < kTasks; ++i) {
        try {
            int v = futs[i].get();
            cs::logf("  [queue] future[", i, "] 结果 = ", v);
        } catch (const std::exception& e) {
            cs::logf("  [queue] future[", i, "] 捕获异常：", e.what());
        }
    }

    for (auto& t : workers) t.join();
    cs::println("");
}

// =====================================================================
// 进阶 1：线程池 submit 接口雏形（为 Capstone1 埋点）。
//   调用方拿到 future<R>，完全不知道任务落在哪个 worker——这正是
//   线程池 submit 的核心形状。用 packaged_task<void()> 做类型擦除。
// =====================================================================
class MiniPool {
public:
    explicit MiniPool(int n) {
        for (int i = 0; i < n; ++i)
            workers_.emplace_back(worker_loop, std::ref(tq_), i);
    }
    ~MiniPool() {
        tq_.close();
        for (auto& t : workers_) t.join();
    }

    // TODO [进阶 1]: submit —— 把任意无参可调用对象打包，返回其 future。
    //   要点：先 get_future()，再把 packaged_task 包进 void() 入队；
    //   返回的 future<R> 让调用方与“在哪执行”彻底解耦。
    //   下面已是正确写法，留作进阶 1 的参考实现：
    template <class F>
    auto submit(F f) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;
        std::packaged_task<R()> task(std::move(f));
        std::future<R> fut = task.get_future();
        tq_.push(std::packaged_task<void()>(
            [t = std::move(task)]() mutable { t(); }));
        return fut;
    }

private:
    TaskQueue tq_;
    std::vector<std::thread> workers_;
};

void demo_mini_pool_submit() {
    cs::println("==== 进阶 1：线程池 submit 接口雏形（Capstone1 埋点）====");

    std::vector<std::future<int>> results;
    {
        MiniPool pool(3);
        for (int i = 0; i < 5; ++i) {
            results.push_back(pool.submit([i] {
                cs::logf("  [pool] task ", i, " on tid ",
                         std::this_thread::get_id());
                return i + 1;
            }));
        }
        // 在 pool 析构（join 全部 worker）之前先收割结果。
        int sum = 0;
        for (auto& f : results) sum += f.get();
        cs::logf("  [pool] 1..5 的 submit 结果之和 = ", sum, "（期望 15）");
    } // pool 析构：close + join 全部 worker

    cs::println("");
}

int main() {
    cs::println("==== D3_packaged_task：打包任务、任务队列与线程池雏形 ====\n");

    demo_basic_packaged_task();  // 必做 1：打包 + 跨线程调用
    demo_task_queue();           // 必做 2/3：任务队列 + worker + 异常
    demo_mini_pool_submit();     // 进阶 1：submit 接口雏形

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
