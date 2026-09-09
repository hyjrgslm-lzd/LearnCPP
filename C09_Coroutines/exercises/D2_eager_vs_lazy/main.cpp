// =====================================================================
// 练习 D-2：eager vs lazy —— 一行 initial_suspend 切换语义
// 文档参考：C09_Coroutines/06-模块D-promise_type全解.md  -> 练习 D-2
// 官方参考：
//   - Lewis Baker, "Understanding the promise type"
//   - P3552R3: std::execution::task<T> 的 lazy 语义设计
//   - cppcoro task.hpp
// 学习要点：
//   1. initial_suspend 是 lazy / eager 的唯一开关。
//   2. eager 隐含"创建 == 立即执行到第一个挂起点"，可能在调用者
//      还没设置 stop_token / continuation 时就开跑。
//   3. P3552 选择 lazy，因为 task 通常先被组合再被启动。
// =====================================================================
#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <print>
#include <stdexcept>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// 模板参数 InitialSuspend 决定 lazy / eager。
//   suspend_always -> lazy
//   suspend_never  -> eager
// ---------------------------------------------------------------------
template <typename T, typename InitialSuspend = std::suspend_always>
struct task {
    struct promise_type {
        T                       result_value{};
        std::exception_ptr      result_exception{};
        std::coroutine_handle<> continuation{};

        task get_return_object() {
            return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        InitialSuspend initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<>
            await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().continuation) return h.promise().continuation;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        void return_value(T v) { result_value = std::move(v); }
        void unhandled_exception() noexcept {
            result_exception = std::current_exception();
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~task() { if (h_) h_.destroy(); }

    T get() {
        if (!h_) throw std::logic_error("empty task");
        if (!h_.done()) h_.resume();
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    // 暴露 handle，便于 eager_start 工厂直接 resume
    auto handle() const noexcept { return h_; }
};

template <typename T> using lazy_task  = task<T, std::suspend_always>;
template <typename T> using eager_task = task<T, std::suspend_never>;

// ---------------------------------------------------------------------
// 一个简单 awaiter：sleep + 立刻 resume，方便观察 multi-step 时序。
// ---------------------------------------------------------------------
struct async_sleep {
    std::chrono::milliseconds dur;
    bool await_ready() const noexcept { return dur <= 0ms; }
    void await_suspend(std::coroutine_handle<> h) const {
        std::thread([h, d = dur] {
            std::this_thread::sleep_for(d);
            h.resume();
        }).detach();
    }
    void await_resume() const noexcept {}
};

// ---------------------------------------------------------------------
// 测试协程：lazy 版与 eager 版只差返回类型
// ---------------------------------------------------------------------
lazy_task<int> lazy_compute() {
    std::println("    [lazy_compute] body 开始");
    co_return 30;
}

eager_task<int> eager_compute() {
    std::println("    [eager_compute] body 开始");
    co_return 30;
}

// 多步协程：包含一次 co_await，用于观察 eager 是跑到第一个挂起点还是 co_return
eager_task<int> multi_step() {
    std::println("    [multi_step] step 1");
    co_await async_sleep{50ms};
    std::println("    [multi_step] step 2");
    co_return 42;
}

// ---------------------------------------------------------------------
// 进阶：混合策略——保留 lazy_task，但对外提供"立即启动"工厂。
// ---------------------------------------------------------------------
template <typename T>
lazy_task<T> eager_start(lazy_task<T> t) {
    // TODO [必做 4]：在返回前 resume() 一次，让协程跑到第一个挂起点。
    if (auto h = t.handle()) h.resume();
    return t;
}

int main() {
    std::println("===== 练习 D-2：eager vs lazy =====\n");

    // ------------------ lazy ------------------
    {
        std::println("[lazy] 创建 task ...");
        auto t = lazy_compute();
        std::println("[lazy] 创建后、get() 前");
        int r = t.get();
        std::println("[lazy] get() 返回 {}\n", r);
        // TODO [必做 1]：观察 "body 开始" 出现在哪一行之后。
    }

    // ------------------ eager ------------------
    {
        std::println("[eager] 创建 task ...");
        auto t = eager_compute();
        std::println("[eager] 创建后、get() 前");
        int r = t.get();
        std::println("[eager] get() 返回 {}\n", r);
        // TODO [必做 2]：观察 "body 开始" 是否在创建那一刻就出现。
    }

    // ------------------ eager + co_await ------------------
    {
        std::println("[multi_step eager] 创建 task ...");
        auto t = multi_step();
        std::println("[multi_step eager] 创建后、get() 前 —— 此时应在第一个挂起点");
        // TODO [必做 3]：分析 step 1 / step 2 各自出现的时刻。
        int r = t.get();
        std::println("[multi_step eager] get() 返回 {}\n", r);
    }

    // ------------------ 混合策略 ------------------
    {
        std::println("[eager_start(lazy)] 创建 ...");
        auto t = eager_start(lazy_compute());
        std::println("[eager_start(lazy)] 创建后 —— 协程已被预先 resume 一次");
        int r = t.get();
        std::println("[eager_start(lazy)] get() 返回 {}\n", r);
    }

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：测量 1000 次 lazy / eager 创建-丢弃的开销差异。
    // TODO [进阶 2]：研究 cppcoro task<T> 的 initial_suspend 选择。
    // TODO [进阶 3]：思考 eager 启动时 stop_token 还没设置的安全隐患。

    std::println("===== Done =====");
    return 0;
}
