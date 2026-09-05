// =====================================================================
// 练习 A-2：co_return 与 lazy task
//   对应文档：02-模块A-三关键字与最小协程.md / 练习 A-2
//   官方参考：
//     - cppreference: https://en.cppreference.com/w/cpp/coroutine/coroutine_handle
//     - cppreference: https://en.cppreference.com/w/cpp/coroutine/suspend_always
//     - Lewis Baker:  https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type
//
// 学习目标：
//   - 看见"创建 task 不执行 / sync_wait 才执行"的惰性
//   - 看见 co_return 的值如何流过 promise.return_value 到 sync_wait 的返回值
// =====================================================================

#include <coroutine_study/lazy_task.hpp>

#include <iostream>
#include <syncstream>
#include <thread>

using coroutine_study::lazy_task;

namespace {

template <class... Args>
void log(const char* tag, Args&&... args) {
    std::osyncstream os{std::cout};
    os << "[" << tag << "] ";
    ((os << args), ...);
    os << "  (tid=" << std::this_thread::get_id() << ")\n";
}

// ─────────────────────────────────────────────────────────────────────
// 必做：compute_async(int x) -> lazy_task<int>
//   三步值变换 + co_return
// ─────────────────────────────────────────────────────────────────────
lazy_task<int> compute_async(int x) {
    // TODO [必做]:
    //   1) log("coro", "step1 enter, x=", x);
    //   2) int step1 = x + 10;            log("coro", "step1 done, =", step1);
    //   3) int step2 = step1 * 2;         log("coro", "step2 done, =", step2);
    //   4) int step3 = step2 - 5;         log("coro", "step3 done, =", step3);
    //   5) co_return step3;
    //
    // 占位实现：让骨架默认可编译运行（直接 co_return x）。
    co_return x;
}

// ─────────────────────────────────────────────────────────────────────
// 必做：等价的同步函数，做完全相同的三步
// ─────────────────────────────────────────────────────────────────────
int compute_sync(int x) {
    // TODO [必做]: 写一个完全等价的同步版本，用同样的日志
    //   对比"调用即执行" vs lazy_task 的"创建不执行"
    int step1 = x + 10;
    int step2 = step1 * 2;
    int step3 = step2 - 5;
    return step3;
}

// ─────────────────────────────────────────────────────────────────────
// 进阶 A：在 step1 后插入一个 co_await std::suspend_always{}
//   观察 sync_wait 内部循环 resume 了几次
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶]: 复制 compute_async，在第一步后加 `co_await std::suspend_always{};`
//   然后在 sync_wait 上加一个计数器（例如修改 lazy_task::sync_wait 临时打印每次 resume）
//   验证多次挂起点也能被一个 sync_wait 消化掉

// ─────────────────────────────────────────────────────────────────────
// 进阶 B：把 int 换成结构体 Request / Response，验证 lazy_task<T> 对非平凡类型的支持
// ─────────────────────────────────────────────────────────────────────
struct Request  { int id; double payload; };
struct Response { int id; double result;  };

// TODO [进阶]: lazy_task<Response> compute_async_struct(Request req);

}  // namespace

int main() {
    log("main", "─── A-2：co_return 与 lazy task ───");

    // ─ 必做 4/5：先创建 task，证明协程体没立刻执行 ──────────────────
    log("main", "构造 task = compute_async(5) —— 此时协程体应一行都没执行");
    auto task = compute_async(5);
    log("main", "task 构造完毕（此前不应看到任何 [coro] 日志）");

    log("main", "调用 sync_wait(std::move(task))");
    int result = coroutine_study::sync_wait(std::move(task));
    log("main", "sync_wait 返回，result = ", result);

    // ─ 必做 6：对照同步版本 ─────────────────────────────────────────
    log("main", "─── 对照：compute_sync(5) ───");
    int sync_result = compute_sync(5);
    log("main", "sync_result = ", sync_result);

    return 0;
}
