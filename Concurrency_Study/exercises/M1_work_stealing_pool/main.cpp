// =====================================================================
// 练习 M-1：工作窃取线程池（work-stealing thread pool）
//   对应文档：Concurrency_Study/16-模块M-工作窃取与结构化并发桥接.md 的 练习 M-1
//
//   学习目标：
//     - 理解【工作窃取（work stealing）】调度：每个 worker 持有【自己的本地
//       双端队列（deque）】，本地任务【LIFO（后进先出）】从【自己这一端
//       （own end / bottom）】push/pop；当本地空了，就去【偷（steal）】别的
//       worker 队列【另一端（other end / top）】的任务；
//     - 体会它相对 Capstone1【单一共享队列 + 一把全局锁】线程池的差异：
//         * 单队列池：所有 worker 抢同一把锁、同一条队列 -> 高争用、易成瓶颈；
//         * 工作窃取池：常态走【无争用的本地队列】，只有空闲时才去偷别人的，
//           把锁争用从“每次取任务”降到“偶尔偷一次”，并实现【负载均衡
//           （load balancing）】——忙的 worker 的积压会被闲的 worker 分担；
//     - 理解【任务粒度（task granularity）】：递归分治（divide and conquer）
//       任务（并行 fib / 二分求和）天然适合 LIFO 本地队列——新切出的子任务
//       压到本地栈顶，本线程下一步就取它，【缓存局部性（cache locality）】好；
//       太细的粒度则调度开销吃掉收益（呼应模块 L 的“并行不是免费午餐”）。
//     - 理解 fork-join 的【合作式等待（cooperative blocking）】：等子任务结果时，
//       【绝不能裸 future.get() 死等】——那会让 worker 线程停转，积压的子任务
//       没人执行 -> 死锁。正确做法是“等的同时继续帮忙跑别的任务”（help on wait），
//       这正是 TBB / Cilk 等真实工作窃取运行时的关键设计。
//
//   实现说明（教学版，刻意简化）：
//     - 真正工业级工作窃取队列是【无锁的 Chase-Lev deque】（见文档“对应官方
//       参考”）。本题【不强求无锁】，用一把 mutex 保护每个 worker 的 std::deque
//       即可——重点是讲清【本地 LIFO + 异端窃取 + 合作式等待】这套【协议】，
//       而非锁的极致优化。
//     - 每个 worker 一把锁、一条 deque：本地 push/pop 走 back（own end），
//       窃取走 front（other end）。两端分离 + 各队列独立锁，常态零跨线程争用。
//
//   骨架说明：关键实现处用 // TODO [必做 N]: / // TODO [进阶 N]: 标记。
//     未填 TODO 处给了“最小占位实现”以保证本文件在 MSVC(VS2026, C++20) 下
//     可直接编译运行——占位版的 push/pop/steal 已是正确实现（建议遮住自己重写），
//     worker 循环与合作式等待也已给出参考实现，程序可正确跑完所有递归分治任务。
//     真正该自己动手的内容见各 TODO 注释。
//
//   官方参考：
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 9 章
//       （高级线程池 / 工作窃取 / 本地任务队列 / run_pending_task 思路）
//     - Chase & Lev, "Dynamic Circular Work-Stealing Deque" (SPAA 2005)
//       —— 无锁工作窃取 deque 的经典论文（本题用加锁简化版，原理同源）
//     - https://en.cppreference.com/w/cpp/container/deque
//
//   编译运行（VS2026, C++20；M1 纯标准库，无需任何第三方库）：
//     cmake --build build-vs2026 --target M1_work_stealing_pool --config Release
//     ./build-vs2026/M1_work_stealing_pool/Release/M1_work_stealing_pool.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <utility>
#include <vector>

// =====================================================================
// WorkStealingDeque：单个 worker 的本地双端队列（教学版，mutex 保护）。
//
//   协议（work-stealing protocol，全题灵魂，先在纸上画一遍）：
//     - own end（自己这一端，这里取 back / bottom）：
//         push(task)  本线程提交新任务 -> push_back
//         pop()       本线程取自己的任务 -> pop_back（【LIFO】：取最新压入的）
//     - other end（别人偷的那一端，这里取 front / top）：
//         steal()     别的 worker 偷任务 -> pop_front（取最老的）
//
//   为什么本地 LIFO、窃取取另一端：
//     - LIFO 让递归分治“刚切出的子任务马上被自己执行”，缓存还热、栈深度可控；
//     - 窃取从另一端取“最老”的任务，通常是更大的、尚未展开的子树，偷一次能
//       让小偷忙很久，减少窃取频率；同时两端分离降低与 owner 的冲突。
// =====================================================================
class WorkStealingDeque {
public:
    using Task = std::function<void()>;

    // owner 端：把新任务压到 back（own end / bottom）。
    void push(Task t) {
        std::lock_guard<std::mutex> lk(mtx_);
        dq_.push_back(std::move(t));
    }

    // owner 端：从 back 取自己的任务（LIFO）。空则返回 nullopt。
    std::optional<Task> pop() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (dq_.empty()) return std::nullopt;
        // TODO [必做 1]: owner 端 LIFO 取任务 —— 从 back（own end）弹出。
        //   要点：本地任务必须从【自己这一端】取，且取【最新压入】的（LIFO），
        //   才能保证递归分治的缓存局部性。这里给出正确实现（无需改动逻辑，
        //   建议你遮住下面两行先自己写一遍）：
        Task t = std::move(dq_.back());
        dq_.pop_back();
        return t;
    }

    // 小偷端：从 front 偷任务（other end / top），取最老的。空则返回 nullopt。
    std::optional<Task> steal() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (dq_.empty()) return std::nullopt;
        // TODO [必做 2]: 窃取端从【另一端 front（top）】取任务（取最老的）。
        //   要点：窃取必须走【与 owner 相反的那一端】，这样两端分离、与 owner
        //   的 push/pop 冲突最小；取最老（front）通常是更大的子树，偷一次划算。
        //   正确实现（同样建议先自己写）：
        Task t = std::move(dq_.front());
        dq_.pop_front();
        return t;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return dq_.empty();
    }

private:
    mutable std::mutex mtx_;
    std::deque<Task> dq_;
};

// =====================================================================
// WorkStealingPool：N 个 worker，每人一条本地 deque。
//
//   提交策略：submit() 把任务投递到“某个”worker 的本地队列。若提交方本身
//   就是池内 worker（递归分治会这样），优先投到自己的本地队列（局部性）；
//   否则轮转投递（round-robin）到各 worker。
//
//   取任务策略（worker_loop 的核心）：
//     1) 先从【自己的本地队列】pop（LIFO，无跨线程争用）；
//     2) 本地空了 -> 轮流去【偷别人】（steal，从对方 front）；
//     3) 都偷不到 -> 短暂让出（yield）后重试；停机且全空则退出。
//
//   合作式等待 wait_for(fut)：等结果时不死睡，而是【继续 try_run_one() 帮忙】，
//   把积压子任务跑掉，避免“worker 全卡在 get() 上而无人执行任务”的死锁。
// =====================================================================
class WorkStealingPool {
public:
    using Task = std::function<void()>;

    explicit WorkStealingPool(std::size_t worker_count =
                                  std::max<std::size_t>(2, std::thread::hardware_concurrency()))
        : count_(worker_count == 0 ? 1 : worker_count),
          queues_(count_) {
        for (std::size_t i = 0; i < count_; ++i)
            queues_[i] = std::make_unique<WorkStealingDeque>();

        workers_.reserve(count_);
        for (std::size_t i = 0; i < count_; ++i) {
            workers_.emplace_back([this, i] {
                tls_index_ = static_cast<long long>(i); // 标记“我是几号 worker”
                worker_loop(i);
            });
        }
        cs::logf("[WSPool] 启动 ", count_, " 个 worker（每人一条本地 deque）。");
    }

    ~WorkStealingPool() {
        done_.store(true, std::memory_order_release);
        for (auto& t : workers_)
            if (t.joinable()) t.join();
        cs::logf("[WSPool] 全部 worker 已退出。窃取次数累计=",
                 steal_count_.load(), "，本地命中累计=", local_hits_.load());
    }

    WorkStealingPool(const WorkStealingPool&)            = delete;
    WorkStealingPool& operator=(const WorkStealingPool&) = delete;

    std::size_t size() const { return count_; }

    // 提交任意可调用对象，返回承载结果/异常的 future。
    template <class F, class... A>
    auto submit(F&& f, A&&... args)
        -> std::future<std::invoke_result_t<F, A...>> {
        using R = std::invoke_result_t<F, A...>;

        auto bound = [func = std::forward<F>(f),
                      ... cargs = std::forward<A>(args)]() mutable -> R {
            return std::invoke(std::move(func), std::move(cargs)...);
        };
        auto task_ptr = std::make_shared<std::packaged_task<R()>>(std::move(bound));
        std::future<R> fut = task_ptr->get_future();

        Task wrapped = [task_ptr]() { (*task_ptr)(); };
        dispatch(std::move(wrapped));
        return fut;
    }

    // 取一个任务并执行：本地优先，否则窃取。跑了任务返回 true，全空返回 false。
    // 既被 worker_loop 用，也被 wait_for() 用（合作式等待靠它“边等边帮忙”）。
    bool try_run_one(std::size_t my, std::mt19937& rng) {
        if (auto t = queues_[my]->pop()) {
            local_hits_.fetch_add(1, std::memory_order_relaxed);
            (*t)();
            return true;
        }
        const std::size_t start = rng() % count_;
        for (std::size_t k = 0; k < count_; ++k) {
            const std::size_t victim = (start + k) % count_;
            if (victim == my) continue;
            if (auto t = queues_[victim]->steal()) {
                steal_count_.fetch_add(1, std::memory_order_relaxed);
                (*t)();
                return true;
            }
        }
        return false;
    }

    // 合作式等待：等 fut 就绪期间，持续帮忙跑任务（绝不裸 get 死等）。
    //   这是 fork-join 在工作窃取池上不死锁的关键。返回 fut 的结果。
    template <class T>
    T wait_for(std::future<T>& fut) {
        // 池内 worker：用本线程身份边等边干活；非池内线程：只能轮询让出。
        const long long self = tls_index_;
        std::mt19937 rng(self >= 0 ? static_cast<unsigned>(self) * 40503u + 7u : 12345u);
        std::size_t my = (self >= 0) ? static_cast<std::size_t>(self) : 0;
        while (fut.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            // TODO [必做 5]: 合作式等待 —— 等结果时不空转，先帮忙跑一个任务。
            //   要点：若本线程是池内 worker，调用 try_run_one(my, rng) 把积压的
            //   子任务跑掉；跑不到任务（或非池内线程）就 yield 让出 CPU 再轮询。
            //   关键认知：这里【绝不能直接 fut.get() 死等】——递归分治时，等待者
            //   本身就是 worker，若它死睡，它队列里的子任务就没人跑，进而死锁。
            //   参考实现（已启用以保证可运行）：
            if (self >= 0) {
                if (!try_run_one(my, rng)) std::this_thread::yield();
            } else {
                std::this_thread::yield();
            }
        }
        return fut.get();
    }

private:
    // 把任务投递到某条本地队列：池内线程优先投自己，否则轮转。
    void dispatch(Task t) {
        if (tls_index_ >= 0) {
            // 递归分治：子任务压到“自己”本地栈顶，下一步本线程就取它（局部性）。
            queues_[static_cast<std::size_t>(tls_index_)]->push(std::move(t));
            return;
        }
        const std::size_t idx =
            next_.fetch_add(1, std::memory_order_relaxed) % count_;
        queues_[idx]->push(std::move(t));
    }

    // 单个 worker 的主循环。
    void worker_loop(std::size_t my) {
        std::mt19937 rng(static_cast<unsigned>(my) * 2654435761u + 1u);

        while (true) {
            // TODO [必做 3 + 必做 4]: worker 主循环 = 本地优先 + 窃取回退。
            //   要点：先尝试 try_run_one(my, rng)（它内部已实现“本地 pop 优先，
            //   本地空则随机起点轮转窃取别的 worker 的 front”）。跑到任务就继续；
            //   一轮都没活干，则检查“停机且全空”退出，否则 yield 让出后重试。
            //   注意 victim 随机化：从随机起点扫，避免空闲 worker 都盯着 0 号、
            //   形成新的争用热点。窃取逻辑见 try_run_one。
            //   参考实现（已启用以保证可编译运行；建议遮住自己重写）：
            if (try_run_one(my, rng)) continue;

            // 既无本地任务也偷不到。停机且全空 -> 退出；否则让出后重试。
            if (done_.load(std::memory_order_acquire) && all_empty()) break;
            std::this_thread::yield();
        }
    }

    bool all_empty() const {
        for (auto& q : queues_)
            if (!q->empty()) return false;
        return true;
    }

    const std::size_t count_;
    std::vector<std::unique_ptr<WorkStealingDeque>> queues_;
    std::vector<std::thread> workers_;
    std::atomic<bool> done_{false};
    std::atomic<std::size_t> next_{0};       // round-robin 投递游标
    std::atomic<long long> steal_count_{0};  // 观测：窃取发生了多少次
    std::atomic<long long> local_hits_{0};   // 观测：本地命中多少次

    // 线程局部：本线程是几号 worker（非池内线程为 -1）。
    static inline thread_local long long tls_index_ = -1;
};

// =====================================================================
// 测试驱动：用递归分治任务验证工作窃取池，并打印观测量。
// =====================================================================

// 必做演示：并行 Fibonacci —— 经典递归分治，天然契合本地 LIFO 队列。
//   注意：fib 本身不是高效算法（指数级），这里【刻意】用它制造大量、不均衡
//   的递归子任务，来压测窃取与负载均衡——而非追求算 fib 的速度。
//   阈值 cutoff 以下退化为串行递归，避免任务粒度过细把调度开销放大（任务粒度）。
static long long parallel_fib(WorkStealingPool& pool, int n, int cutoff) {
    if (n <= cutoff) {
        // 粒度太细就串行算：递归分治剪枝，控制任务数量。
        long long a = 0, b = 1;
        if (n < 2) return n;
        for (int i = 2; i <= n; ++i) { long long c = a + b; a = b; b = c; }
        return b;
    }
    // 切成两个子任务：一个提交给池（可能被别人偷走），一个本线程递归算。
    auto left = pool.submit(parallel_fib, std::ref(pool), n - 1, cutoff);
    long long right = parallel_fib(pool, n - 2, cutoff);
    // 合作式等待：等 left 期间继续帮忙跑任务，绝不裸 get 死等（避免死锁）。
    return pool.wait_for(left) + right;
}

static void demo_parallel_fib(WorkStealingPool& pool) {
    cs::println("======== 必做 4：递归分治（并行 Fibonacci）压测窃取 ========");
    constexpr int kN = 32;
    constexpr int kCutoff = 24; // n<=24 串行，制造适量子任务

    auto fut = pool.submit(parallel_fib, std::ref(pool), kN, kCutoff);
    const long long got = pool.wait_for(fut);

    // 串行基线：闭式迭代 fib，校验正确性。
    long long a = 0, b = 1, expected = kN;
    if (kN >= 2) { for (int i = 2; i <= kN; ++i) { long long c = a + b; a = b; b = c; } expected = b; }

    cs::logf("[main] parallel_fib(", kN, ") = ", got, "，期望 ", expected,
             " -- ", (got == expected ? "一致 OK" : "不一致 MISMATCH"));
    cs::println("");
}

// 进阶演示：并行求和的递归二分（fork-join 风格），验证负载均衡更直观。
static long long parallel_sum(WorkStealingPool& pool, long long lo, long long hi, long long cutoff) {
    const long long len = hi - lo;
    if (len <= cutoff) {
        long long s = 0;
        for (long long i = lo; i < hi; ++i) s += i;
        return s;
    }
    const long long mid = lo + len / 2;
    // TODO [进阶 1]: 把“左半”提交给池、右半本线程递归（fork-join 二分）。
    //   要点：这正是工作窃取最擅长的形态——把一半工作 fork 到本地队列，另一半
    //   自己接着切，直到粒度 <= cutoff。空闲 worker 会来偷走积压的左半子树，
    //   从而把不均衡的工作均摊（负载均衡）。等结果同样要用 pool.wait_for()
    //   合作式等待。完成后请遮住下面参考实现重写。
    //
    //   参考实现（已启用以保证可编译运行）：
    auto left = pool.submit(parallel_sum, std::ref(pool), lo, mid, cutoff);
    long long right = parallel_sum(pool, mid, hi, cutoff);
    return pool.wait_for(left) + right;
}

static void demo_parallel_sum(WorkStealingPool& pool) {
    cs::println("======== 进阶：递归二分并行求和（fork-join）========");
    constexpr long long kN = 2'000'000;
    constexpr long long kCutoff = 50'000;

    auto fut = pool.submit(parallel_sum, std::ref(pool), 0LL, kN, kCutoff);
    const long long got = pool.wait_for(fut);
    const long long expected = (kN - 1) * kN / 2; // 0+1+...+(N-1)

    cs::logf("[main] sum(0..", kN - 1, ") = ", got, "，期望 ", expected,
             " -- ", (got == expected ? "一致 OK" : "不一致 MISMATCH"));
    cs::println("");
}

int main() {
    cs::println("==== M1_work_stealing_pool：工作窃取线程池 ====");
    cs::println("  每个 worker 一条本地 deque：本地 LIFO push/pop（own end），");
    cs::println("  空闲时从别的 worker 队列【另一端】偷（steal）。");
    cs::println("  对比 Capstone1 的单队列+全局锁：常态走本地、零跨线程争用，");
    cs::println("  仅在空闲时偷取，实现负载均衡。\n");

    {
        // 控制 worker 数（演示用 4 个即可看清窃取，且更稳定）。
        const std::size_t nw =
            std::min<std::size_t>(4, std::max<std::size_t>(2, std::thread::hardware_concurrency()));
        WorkStealingPool pool(nw);

        demo_parallel_fib(pool);  // 必做 4：递归分治压测窃取
        demo_parallel_sum(pool);  // 进阶 1：fork-join 二分求和

        cs::logf("[main] 池析构：置停机标志，等所有 worker 排空并退出。");
    } // pool 析构 -> 停机 + join

    cs::println("\n观察提示：");
    cs::println("  - 析构日志里的“窃取次数/本地命中”比例反映了负载是否均衡：");
    cs::println("    本地命中应远多于窃取（常态走本地），窃取只在不均衡时发生。");
    cs::println("  - 对比 Capstone1：那里所有 worker 抢【同一把锁/同一条队列】；");
    cs::println("    这里每人一条队列，仅偷取时才偶发跨线程加锁。");
    cs::println("  - fork-join 用 pool.wait_for()【合作式等待】：等结果时继续帮忙跑任务，");
    cs::println("    绝不裸 future.get() 死等——否则 worker 全卡在 get 上会死锁。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
