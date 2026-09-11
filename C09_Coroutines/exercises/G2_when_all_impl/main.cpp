// G-2 实现 when_all<T...>
// 文档参考：09-模块G-symmetric_transfer与高级task.md 「练习 G-2」
// 官方参考：
//   - Lewis Baker "C++ coroutines: Composing coroutines"
//   - cppcoro when_all.hpp
//   - P2300R10 std::execution::when_all
//
// 目标：让 N 个子 task 先全部启动，再统一等待完成；最后一个完成的子 task
//      唤醒等待者；用 std::tuple 汇合结果；错误处理采用 fail-delay：所有子 task
//      完成后若有任何异常，传播第一个。
//
// 主任务：实现可编译的 2-task 固定版 `when_all_2`。
// 进阶  ：用 std::index_sequence_for 把索引带入 fold expression，做变参版。

#include <atomic>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

// ============ 复用 lazy_task<T> ============
template <typename T>
struct lazy_task {
    using value_type = T;

    struct promise_type {
        T result_value{};
        std::exception_ptr result_exception;
        std::coroutine_handle<> continuation;

        lazy_task get_return_object() noexcept {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
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
        void unhandled_exception() noexcept { result_exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h_;
    explicit lazy_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task(const lazy_task&) = delete;
    ~lazy_task() { if (h_) h_.destroy(); }

    bool await_ready() noexcept { return false; }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
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

// ============ when_all_2：可编译的 2-task 固定版（主任务） ============
// 设计：这里仍是 starter 的顺序 drain 待改造起点；它只能证明顺序收束，
//      不能证明真正 fan-out。目标实现应先启动所有子 task，再由 barrier 汇合。
template <typename T0, typename T1>
lazy_task<std::tuple<T0, T1>> when_all_2(lazy_task<T0> t0, lazy_task<T1> t1)
{
    // Starter 占位：顺序 drain 两个 task；请改造成先启动所有分支再等待。
    if (!t0.done()) t0.resume();
    while (!t0.done()) t0.h_.resume();

    if (!t1.done()) t1.resume();
    while (!t1.done()) t1.h_.resume();

    // 收集结果（fail-delay）：第一个异常胜出
    std::exception_ptr err;
    auto& p0 = t0.h_.promise();
    auto& p1 = t1.h_.promise();
    if (p0.result_exception) err = p0.result_exception;
    else if (p1.result_exception) err = p1.result_exception;

    if (err) std::rethrow_exception(err);

    co_return std::make_tuple(std::move(p0.result_value),
                              std::move(p1.result_value));
}

// ============ 进阶：变参 when_all 设计示意（不可直接编译） ============
//
// template <typename... Tasks>
// auto when_all(Tasks... tasks) {
//     // 共享状态：原子计数 + 结果槽位 + 第一个异常
//     struct shared_state {
//         std::atomic<int> remaining{sizeof...(Tasks)};
//         std::tuple<typename Tasks::value_type...> results{};
//         std::exception_ptr first_error{};
//         std::coroutine_handle<> waiter{};
//     };
//
//     // 用 std::index_sequence_for 把索引 I 带入 fold——
//     // 关键是把"硬编码 0"替换为"模板参数 I"，每个子 task 知道自己的槽位。
//     auto state = std::make_shared<shared_state>();
//     [&]<std::size_t... Is>(std::index_sequence<Is...>) {
//         (void)std::initializer_list<int>{
//             (when_all_child<Is>(state.get(), std::move(tasks)), 0)...
//         };
//     }(std::index_sequence_for<Tasks...>{});
//     // 然后构造一个等待 sender / awaitable，用 state->remaining == 0 触发 resume。
// }

// ============ 测试场景 ============
lazy_task<int>         fetch_int()    { co_return 10; }
lazy_task<std::string> fetch_string() { co_return std::string{"hello"}; }
lazy_task<int>         fetch_fails()  {
    throw std::runtime_error("intentional");
    co_return 0;   // unreachable
}

struct saved_suspend {
    int id;
    static inline std::coroutine_handle<> handles[2]{};
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { handles[id] = h; }
    void await_resume() const noexcept {}
};

static inline int probe_started = 0;
static inline int probe_completed = 0;
static inline bool first_completed_before_second_started = false;

lazy_task<int> probe_branch(int id) {
    ++probe_started;
    co_await saved_suspend{id};
    if (id == 0 && probe_started < 2) first_completed_before_second_started = true;
    ++probe_completed;
    co_return id + 1;
}

lazy_task<int> probe_parent() {
    auto [a, b] = co_await when_all_2(probe_branch(0), probe_branch(1));
    co_return a + b;
}

// 注：本题内自定义 lazy_task 的 promise 用 T result_value{} + return_value(T)，
// 对 T=void 不合法。此处用 lazy_task<int> + co_return 0 规避 void 实例化。
lazy_task<int> demo_when_all_2() {
    auto [a, b] = co_await when_all_2(fetch_int(), fetch_string());
    std::printf("[when_all_2] got int=%d string=\"%s\"\n", a, b.c_str());
    co_return 0;
}

lazy_task<int> demo_when_all_2_with_error() {
    try {
        auto [a, b] = co_await when_all_2(fetch_fails(), fetch_string());
        std::printf("[when_all_2_err] got int=%d string=\"%s\"\n", a, b.c_str());
    } catch (const std::exception& e) {
        std::printf("[when_all_2_err] caught: %s\n", e.what());
    }
    co_return 0;
}

int main() try
{
    std::printf("===== G-2: when_all =====\n\n");

    {
        std::printf("--- 测试 1：when_all_2 成功路径 ---\n");
        auto t = demo_when_all_2();
        t.resume();
        while (!t.done()) t.h_.resume();
    }

    {
        std::printf("\n--- 测试 2：when_all_2 错误路径（fail-delay）---\n");
        auto t = demo_when_all_2_with_error();
        t.resume();
        while (!t.done()) t.h_.resume();
    }

    {
        std::printf("\n--- 测试 3：starter fan-out 检查 ---\n");
        auto t = probe_parent();
        t.resume();
        check(!first_completed_before_second_started,
              "TODO: when_all_2 must start both child tasks before draining either one");
        check(probe_started == 2, "TODO: when_all_2 must fan out both branches");
        if (saved_suspend::handles[0]) saved_suspend::handles[0].resume();
        if (saved_suspend::handles[1]) saved_suspend::handles[1].resume();
        check(t.done(), "last child completion must resume the parent");
        check(t.await_resume() == 3 && probe_completed == 2,
              "when_all_2 must collect both child results after the barrier");
    }

    // TODO [必做]：在笔记中画 when_all 的状态转换图：
    //   remaining=N → 每个分支完成时 -1 → remaining=0 → waiter.resume() → await_resume 返回 tuple
    // TODO [必做]：列出三种错误合并策略（fail-fast / fail-delay / aggregation）
    //   各自适合的场景。
    // TODO [进阶]：实现变参 when_all——用 std::index_sequence_for 把索引带入 fold。
    // TODO [进阶]：实现 when_any（第一个完成胜出，其余取消）。
    // TODO [进阶]：把 run_loop/manual_event 版改为多线程并行（提交到线程池）。

    std::printf("\n===== Done =====\n");
    return 0;
} catch (const std::exception& e) {
    std::cerr << "starter check failed: " << e.what() << '\n';
    return 1;
}
