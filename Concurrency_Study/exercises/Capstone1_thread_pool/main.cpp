// =====================================================================
// 练习 Capstone1_thread_pool：第一阶段结课 · 线程池与生产者-消费者
//   对应文档：Concurrency_Study/06-第一阶段结课-线程池与生产者消费者.md
//
//   学习目标（综合阶段一全部核心原语）：
//     - std::jthread (C++20)         —— 持有 N 个常驻 worker，析构自动 join；
//     - std::mutex (C++11)           —— 保护任务队列这块共享可变状态；
//     - std::condition_variable      —— worker 空闲时睡眠、有任务/停机时唤醒；
//     - std::packaged_task (C++11)   —— 把任意可调用对象打包，解耦结果与执行；
//     - std::future (C++11)          —— 把返回值与异常从 worker 回传给提交方；
//     - std::stop_token (C++20)      —— 优雅停机：先停收新任务，排空在途任务再退出。
//
//   本项目本质：把练习 C-2（有界阻塞队列）、C-3（stop_token 可中断等待）、
//   D-3（packaged_task）三题缝合成一个可复用的固定大小线程池。
//
//   骨架说明：关键实现处用 // TODO [必做 N]: / // TODO [进阶 N]: 标记。
//   未填 TODO 处给了“最小占位实现”以保证本文件在 MSVC(VS2026, C++20) 下
//   可直接编译运行——占位版线程池退化为“在提交线程内同步执行任务”，
//   因此输出仍然正确，只是没有真正的并发。真正该实现的内容见各 TODO 注释，
//   参考代码以注释形式给出，填写时把占位段替换为它即可。
//
//   官方参考：
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 9 章（线程池）
//     - https://en.cppreference.com/w/cpp/thread/jthread
//     - https://en.cppreference.com/w/cpp/thread/packaged_task
//     - https://en.cppreference.com/w/cpp/thread/condition_variable
//     - https://en.cppreference.com/w/cpp/thread/stop_token
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target Capstone1_thread_pool --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// =====================================================================
// 固定大小线程池。
//
//   并发拓扑（务必先在纸上画一遍）：
//     提交端 submit() ──打包 packaged_task──▶ [ mutex 保护的有界任务队列 ]
//                                                  │ notify_one
//                                                  ▼
//                          N 个 worker(jthread) ──取任务──▶ 解锁后执行
//                                                  │
//                                          future 承载结果/异常回传提交端
//
//   两个关键谓词（贯穿全题）：
//     - worker “继续干活/被唤醒”的谓词：队列非空 || 已停机；
//     - worker “退出循环”的条件：     已停机 && 队列已排空。
//   注意这两者不同：前者决定何时醒，后者决定何时彻底退出（先排空再退出）。
// =====================================================================
class ThreadPool {
public:
    // 任务在队列里以“无参 void()”的形式存在。packaged_task 只能移动不能拷贝，
    // 因此理想承载类型是 std::move_only_function<void()>（C++23）。为最大化
    // 跨编译器可用性，这里用 std::function<void()> + 一层 shared_ptr 间接持有
    // packaged_task（shared_ptr 可拷贝，从而让闭包可拷贝、能塞进 std::function）。
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t worker_count, std::size_t queue_capacity)
        : capacity_(queue_capacity == 0 ? 1 : queue_capacity) {
        if (worker_count == 0) worker_count = 1;

        // -------------------------------------------------------------
        // TODO [必做 4]: 启动 worker_count 个常驻 worker 线程。
        //   真正实现：用 std::jthread，把成员函数 worker_loop 绑定为线程体；
        //   jthread 会把自己的 std::stop_token 作为首参传入（进阶 1 用到）。
        //
        //   参考代码（必做版，自带停机布尔；进阶版改用 stop_token）：
        //     workers_.reserve(worker_count);
        //     for (std::size_t i = 0; i < worker_count; ++i) {
        //         workers_.emplace_back([this](std::stop_token st) {
        //             worker_loop(st);
        //         });
        //     }
        //
        // 占位实现：不启动任何线程。提交的任务将在 submit() 内被同步执行
        //   （见下方 enqueue 占位），从而保证本骨架可编译运行、输出正确。
        // -------------------------------------------------------------
        cs::logf("[ThreadPool] 占位模式：未启动 worker，任务将同步执行。worker=",
                 worker_count, " capacity=", capacity_);
        (void)worker_count;
    }

    ~ThreadPool() {
        // -------------------------------------------------------------
        // TODO [必做 6]: 优雅停机（graceful shutdown）。
        //   真正实现：
        //     1) 置停机标志（或对每个 jthread request_stop），不再收新任务；
        //     2) notify_all 唤醒所有卡在 wait 上的 worker；
        //     3) jthread 析构自动 join —— 但通知必须在 join 发生前发出。
        //   worker 谓词把“已停机”纳入唤醒条件，但只有“队列也排空”后才退出，
        //   从而保证在途任务不被丢弃（drain 策略）。
        //
        //   参考代码（必做版）：
        //     {
        //         std::lock_guard<std::mutex> lk(mtx_);
        //         stopping_ = true;
        //     }
        //     not_empty_.notify_all();
        //     // workers_ 是 jthread，离开作用域时自动 request_stop + join。
        //
        // 占位实现：没有 worker、没有在途任务，无需停机动作。
        // -------------------------------------------------------------
        cs::logf("[ThreadPool] 占位模式析构：无 worker 需停机。");
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // -----------------------------------------------------------------
    // submit：提交任意可调用对象 f 及其参数 args，返回承载结果的 future。
    //   返回类型 R = std::invoke_result_t<F, A...>，异常也经该 future 传播。
    //   该签名是验收的一部分，请勿改动语义。
    // -----------------------------------------------------------------
    template <class F, class... A>
    auto submit(F&& f, A&&... args)
        -> std::future<std::invoke_result_t<F, A...>> {
        using R = std::invoke_result_t<F, A...>;

        // TODO [必做 3]: 用 packaged_task 打包 f(args...)。
        //   要点：
        //     - 先把可调用对象与参数绑定成一个无参可调用对象（注意完美转发/
        //       移动捕获），再交给 packaged_task<R()>；
        //     - 取出它的 future（这一步必须在把 task 移进队列“之前”做，
        //       否则 task 一旦被 worker 执行/销毁，再取 future 就晚了）；
        //     - packaged_task 不可拷贝，用 shared_ptr 间接持有，便于塞进
        //       std::function（可拷贝要求）。
        //
        //   这里给出的就是“正确实现”（打包部分无需改动）：
        auto bound = [func = std::forward<F>(f),
                      ... cargs = std::forward<A>(args)]() mutable -> R {
            return std::invoke(std::move(func), std::move(cargs)...);
        };
        auto task_ptr =
            std::make_shared<std::packaged_task<R()>>(std::move(bound));
        std::future<R> fut = task_ptr->get_future();

        // 包成无参 void() 任务（执行即运行 packaged_task）。
        Task wrapped = [task_ptr]() { (*task_ptr)(); };

        enqueue(std::move(wrapped));
        return fut;
    }

private:
    // -----------------------------------------------------------------
    // enqueue：把一个已打包好的 void() 任务放入队列。
    // -----------------------------------------------------------------
    void enqueue(Task task) {
        // -------------------------------------------------------------
        // TODO [必做 2 + 必做 5]: 加锁入队 + 背压 + 通知。
        //   真正实现（有界队列、阻塞式背压）：
        //     {
        //         std::unique_lock<std::mutex> lk(mtx_);
        //         // 满则等待“非满 或 已停机”（背压：练习 C-2 技法）
        //         not_full_.wait(lk, [&]{ return queue_.size() < capacity_ || stopping_; });
        //         if (stopping_)
        //             throw std::runtime_error("ThreadPool 已停机，拒绝新任务");
        //         queue_.push(std::move(task));
        //     }
        //     not_empty_.notify_one();   // 新增一个任务，唤醒一个 worker
        //
        // 占位实现：没有 worker 来消费队列，直接在“提交线程”内同步执行，
        //   保证任务一定被运行、future 一定被就绪，从而骨架可运行且输出正确。
        //   注意：占位版没有任何并发，仅用于让结构跑起来。
        // -------------------------------------------------------------
        task(); // 同步执行（占位）。真正实现见上方注释。
    }

    // -----------------------------------------------------------------
    // worker_loop：每个 worker 线程的主体。
    //   必做版用成员布尔 stopping_ 作为停机信号；进阶版改用 stop_token。
    // -----------------------------------------------------------------
    void worker_loop([[maybe_unused]] std::stop_token stoken) {
        cs::logf("[worker] 启动");
        // -------------------------------------------------------------
        // TODO [必做 4]: worker 取任务-执行循环（禁止忙等）。
        //   真正实现（必做版，成员布尔停机）：
        //     while (true) {
        //         Task task;
        //         {
        //             std::unique_lock<std::mutex> lk(mtx_);
        //             // 醒来谓词：队列非空 或 已停机
        //             not_empty_.wait(lk, [&]{ return !queue_.empty() || stopping_; });
        //             // 退出条件：已停机 且 队列已排空（先排空再退出）
        //             if (stopping_ && queue_.empty()) break;
        //             task = std::move(queue_.front());
        //             queue_.pop();
        //         }                       // —— 先解锁
        //         not_full_.notify_one(); // 空出一个位置，唤醒可能在等的提交方
        //         task();                 // —— 再在锁外执行任务（关键！）
        //     }
        //
        //   TODO [进阶 1]: 改用 std::condition_variable_any + stop_token 的
        //     可中断 wait（练习 C-3 技法），把 not_empty_ 换成 _any 版本：
        //       not_empty_any_.wait(lk, stoken, [&]{ return !queue_.empty(); });
        //     再用 (stoken.stop_requested() && queue_.empty()) 作为退出条件。
        //     好处：jthread 析构自动 request_stop 即触发停机，无需自定义布尔。
        //
        // 占位实现：worker 不会被启动（见构造函数占位），此函数体此处留空，
        //   仅打印一条启动日志以示意结构。真正实现请替换为上面的循环。
        // -------------------------------------------------------------
        cs::logf("[worker] 占位模式：未参与调度（任务在提交线程同步执行）。");
    }

    const std::size_t capacity_;
    std::mutex mtx_;
    std::condition_variable not_empty_; // worker 等它：队列“非空”
    std::condition_variable not_full_;  // 提交方等它：队列“非满”（背压）
    std::queue<Task> queue_;
    bool stopping_ = false;             // 必做版停机标志（进阶版可改用 stop_token）
    std::vector<std::jthread> workers_; // N 个常驻 worker（占位模式下为空）
};

// =====================================================================
// 测试驱动：在 main() 内用 stdout 验证两条路径。
// =====================================================================

// 必做 7：返回值路径——提交一批平方任务，收集 future 求和并与公式比对。
static void demo_return_values(ThreadPool& pool) {
    cs::println("======== 必做 7：返回值经 future 回传 ========");
    constexpr int kN = 100;

    std::vector<std::future<long long>> futures;
    futures.reserve(kN);
    for (int i = 0; i < kN; ++i) {
        futures.push_back(pool.submit([i]() -> long long {
            return static_cast<long long>(i) * i;
        }));
    }

    long long sum = 0;
    for (auto& f : futures) sum += f.get();

    // sum_{i=0}^{n-1} i^2 = (n-1)n(2n-1)/6
    const long long n = kN;
    const long long expected = (n - 1) * n * (2 * n - 1) / 6;
    cs::logf("[main] sum(i*i, i=0..", kN - 1, ") 期望 ", expected, "，实际 ", sum,
             " -- ", (sum == expected ? "一致 OK" : "不一致 MISMATCH"));
    cs::println("");
}

// 必做 8：异常路径——提交会抛异常的任务，验证异常经 future.get() 重新抛出。
static void demo_exception_propagation(ThreadPool& pool) {
    cs::println("======== 必做 8：异常经 future 传播 ========");

    auto fut = pool.submit([]() -> int {
        throw std::runtime_error("boom from task");
        return 0; // 不可达
    });

    try {
        int v = fut.get();
        cs::logf("[main] 异常未传播，意外取到值 ", v, " -- MISMATCH");
    } catch (const std::exception& e) {
        cs::logf("[main] future.get() 捕获到异常：", e.what(), " -- OK");
    }
    cs::println("");
}

int main() {
    cs::println("==== Capstone1：固定大小线程池 ====\n");

    const std::size_t workers =
        std::max<std::size_t>(2, std::thread::hardware_concurrency());
    const std::size_t capacity = 64;
    cs::logf("[main] 启动线程池：worker=", workers, ", capacity=", capacity);

    {
        ThreadPool pool(workers, capacity);

        demo_return_values(pool);        // 必做 7
        demo_exception_propagation(pool); // 必做 8

        cs::logf("[main] 线程池析构：优雅停机（排空在途任务后退出）。");
    } // pool 析构 → 必做 6 优雅停机

    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
