// I-3 Folly coro Task 与 SafeTask
// 文档参考：11-模块I-真实异步IO与并发框架.md 「练习 I-3」
// 官方参考：
//   - folly/experimental/coro/Task.h
//   - folly/experimental/coro/SafeTask.h
//   - folly/experimental/coro/NowTask.h
//   - folly/experimental/coro/Mutex.h
//   - folly/experimental/coro/AsyncScope.h
//   - Lewis Baker "Structured Concurrency" CppCon 2019
//
// 目标：把 lazy_task 升级为 Folly 风格的 SafeTask——通过编译期 static_assert
//      禁止裸引用 / 顶层 const、运行时一次性 await 断言、Executor 注入、
//      FIFO 公平 CoMutex。
//
// 注意：本文件中标注 "// 以下是 Folly API 概念示意——不可直接编译" 的代码段
//      仅作为阅读引导，不在 main 中链接。骨架只让"自己写的简化版"可编译运行。

#include <atomic>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <functional>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

// ============ 简化 my_safe_task<T>：编译期约束 ============
template <typename T>
struct my_safe_task {
    // 编译期约束 1：拒绝裸引用（防止悬空引用）
    static_assert(!std::is_reference_v<T>,
        "SafeTask does not allow T to be a reference. "
        "Use std::reference_wrapper<T> if you need a reference handle.");

    // 编译期约束 2：拒绝顶层 const（按值返回时无意义）
    static_assert(!std::is_const_v<T>,
        "SafeTask does not allow T to be top-level const. "
        "Top-level const is meaningless on a return-by-value type.");

    struct promise_type {
        T result_value{};
        std::exception_ptr result_exception;
        std::coroutine_handle<> continuation;
        std::atomic<bool> awaited{false};   // 运行时一次性断言

        my_safe_task get_return_object() noexcept {
            return my_safe_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto cont = h.promise().continuation;
                    return cont ? cont : std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
        void return_value(T v) noexcept { result_value = std::move(v); }
        void unhandled_exception() noexcept {
            result_exception = std::current_exception();
        }
    };

    std::coroutine_handle<promise_type> h_;
    explicit my_safe_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    my_safe_task(my_safe_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    my_safe_task(const my_safe_task&) = delete;
    ~my_safe_task() { if (h_) h_.destroy(); }

    bool await_ready() noexcept { return false; }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
        // 运行时一次性断言：fire-and-forget 语义保护
        bool expected = false;
        if (!h_.promise().awaited.compare_exchange_strong(expected, true)) {
            std::fprintf(stderr,
                "[SafeTask] FATAL: same task co_awaited more than once\n");
            std::abort();
        }
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    void resume() { if (h_) h_.resume(); }
    bool done() const noexcept { return h_ && h_.done(); }
};

// ============ 极简 Executor ============
struct simple_executor {
    std::queue<std::function<void()>> q;
    void post(std::function<void()> fn) { q.push(std::move(fn)); }
    void run_until_idle() {
        while (!q.empty()) {
            auto fn = std::move(q.front());
            q.pop();
            fn();
        }
    }
};

// 概念示意：co_via_if_async（Folly 模式的简化）
// 单线程 executor 的"切回当前 ex"是 no-op；多线程 executor 才真有切换语义
struct via_awaiter {
    simple_executor* ex_;
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        ex_->post([h] { h.resume(); });
    }
    void await_resume() noexcept {}
};

// ============ FIFO 公平 CoMutex ============
struct co_mutex {
    bool locked = false;
    std::queue<std::coroutine_handle<>> waiters;

    struct lock_awaiter {
        co_mutex& mtx;

        bool await_ready() noexcept {
            if (!mtx.locked) {
                mtx.locked = true;
                return true;        // 没人占着 → 直接拿到锁
            }
            return false;
        }
        void await_suspend(std::coroutine_handle<> h) noexcept {
            // FIFO 入队：尾插
            mtx.waiters.push(h);
        }
        void await_resume() noexcept {
            // 被 unlock 唤醒时已是锁的持有者（locked 仍为 true）
        }
    };

    lock_awaiter lock() noexcept { return {*this}; }

    void unlock() {
        if (waiters.empty()) {
            locked = false;
        } else {
            auto next = waiters.front();
            waiters.pop();
            // locked 保持 true → 锁所有权传给下一个 waiter
            next.resume();
        }
    }
};

// ============ 概念示意：Folly co_cleanup（不可直接编译）============
//
// 以下是 Folly API 概念示意——不可直接编译
//
// folly::coro::Task<void> with_cleanup() {
//     auto cleanup = folly::coro::co_cleanup([&]() -> folly::coro::Task<void> {
//         std::printf("cleanup executed\n");
//         co_return;
//     });
//     co_await some_io();
//     // co_cleanup 析构时自动 co_await 注册的回调
// }

// ============ 概念示意：Folly Task 的真正声明（不可直接编译）============
//
// 以下是 Folly API 概念示意——不可直接编译
//
// #include <folly/experimental/coro/Task.h>
// folly::coro::Task<int> folly_compute(folly::Executor* ex) {
//     co_await folly::coro::co_reschedule_on_current_executor;
//     co_return 42;
// }

// ============ 测试 ============
my_safe_task<int> compute() { co_return 42; }
my_safe_task<int> doubled() {
    int v = co_await compute();
    co_return v * 2;
}

my_safe_task<void> mutex_user(co_mutex& m, int id, std::vector<int>& log) {
    co_await m.lock();
    log.push_back(id);
    std::printf("[user %d] entered critical section\n", id);
    m.unlock();
    co_return;
}

int main()
{
    std::printf("===== I-3: SafeTask + CoMutex =====\n\n");

    {
        std::printf("--- 测试 1：编译期约束 ---\n");
        // 下行解开注释会触发 static_assert：
        //   my_safe_task<int&>      ref_test;       // ❌ reference
        //   my_safe_task<const int> const_test;     // ❌ top-level const
        std::printf("  static_assert 已就位（解开注释观察编译错误）\n");
    }

    {
        std::printf("\n--- 测试 2：基本 co_await ---\n");
        auto t = doubled();
        t.resume();
        while (!t.done()) t.h_.resume();
        std::printf("  result = %d (expect 84)\n",
                    t.h_.promise().result_value);
    }

    {
        std::printf("\n--- 测试 3：CoMutex FIFO 公平性 ---\n");
        co_mutex m;
        std::vector<int> log;
        // 启动 3 个 user，按序 co_await lock
        auto u0 = mutex_user(m, 0, log);
        auto u1 = mutex_user(m, 1, log);
        auto u2 = mutex_user(m, 2, log);
        // 依次 resume：u0 立即拿锁；u1/u2 排队
        u0.resume();          // u0 拿锁，进入临界区，unlock 后立即唤醒 u1...
        u1.resume();          // u1 此时还在 await_ready 检查（已加锁）→ 入队
        u2.resume();          // 同上 → 入队
        // 真正驱动：unlock 链上的 resume 在 u0 的 unlock 内已发生
        // 但 u1/u2 在 await_suspend 中入队并未 resume，需要外部 drain
        while (!u0.done()) u0.h_.resume();
        while (!u1.done()) u1.h_.resume();
        while (!u2.done()) u2.h_.resume();
        std::printf("  FIFO order: ");
        for (auto x : log) std::printf("%d ", x);
        std::printf("(expect 0 1 2 in some FIFO realization)\n");
    }

    // TODO [必做]：解开测试 1 中两行注释，确认编译错误信息可读。
    // TODO [必做]：列举 SafeTask 比 lazy_task 更安全的 3 个设计决策。
    // TODO [进阶]：把 std::queue 换成侵入式链表，消除堆分配。
    // TODO [进阶]：阅读 folly::coro::NowTask 源码，复述其设计意图。

    std::printf("\n===== Done =====\n");
    return 0;
}
