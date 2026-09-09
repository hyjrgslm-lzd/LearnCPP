// =====================================================================
// 练习 A-1：第一个 generator
//   对应文档：02-模块A-三关键字与最小协程.md / 练习 A-1
//   官方参考：
//     - cppreference: https://en.cppreference.com/w/cpp/coroutine/generator
//     - P2502R2:      https://wg21.link/P2502R2
//     - Lewis Baker:  https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
//
// 学习目标：
//   - 体感 co_yield 的惰性（迭代器不推进，协程体不前进）
//   - 用 std::views::take 截取无限/有限生成器
//   - 看到日志在 main 与协程体之间交替出现
// =====================================================================

#include <generator>
#include <iostream>
#include <ranges>
#include <string>
#include <thread>
#include <syncstream>

namespace {

// 小工具：线程安全的日志，统一格式 "[tag] msg"
template <class... Args>
void log(const char* tag, Args&&... args) {
    std::osyncstream os{std::cout};
    os << "[" << tag << "] ";
    ((os << args), ...);
    os << "  (tid=" << std::this_thread::get_id() << ")\n";
}

// ─────────────────────────────────────────────────────────────────────
// 必做 1：fibonacci(N) —— 返回 std::generator<int>
//   要求：用 co_yield 依次产出 N 项斐波那契
//   建议：a=0, b=1; 然后 co_yield a; co_yield b; 之后循环 co_yield a+b
// ─────────────────────────────────────────────────────────────────────
std::generator<int> fibonacci(int n) {
    // TODO [必做]: 实现一个产出前 N 项斐波那契的 generator
    //   1) 在协程体的开头打印一条日志（提示协程帧第一次 resume 的时刻）
    //   2) 用 co_yield 依次产出 0, 1, 1, 2, 3, ...
    //   3) 在每次 co_yield 之前再打 1 行日志，证明"消费者推进 -> 生产者执行"是交替的
    //
    // 占位实现：让骨架默认可编译运行（产出 0..n-1）。
    for (int i = 0; i < n; ++i) {
        co_yield i;
    }
}

// ─────────────────────────────────────────────────────────────────────
// 必做 5：read_lines() —— 返回 std::generator<std::string>
//   模拟逐行读取，5~8 行
// ─────────────────────────────────────────────────────────────────────
std::generator<std::string> read_lines() {
    // TODO [必做]: 用 co_yield 模拟产出 5~8 行文本
    //   每次 yield 前后各加一行日志，观察行号
    co_yield std::string{"<line placeholder 0>"};
    co_yield std::string{"<line placeholder 1>"};
}

// ─────────────────────────────────────────────────────────────────────
// 进阶 A：无限 fibonacci，配合 views::take(15)
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶]: 实现 std::generator<int> fibonacci_inf(); 然后在 main 中用
//   std::views::take(fibonacci_inf(), 15) 截取前 15 项

// ─────────────────────────────────────────────────────────────────────
// 进阶 B：filter_even —— 接收 std::generator<int>&& 包装
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶]: 实现 std::generator<int> filter_even(std::generator<int>&& src);
//   只 co_yield 偶数项；演示 generator 组合（一个 generator 喂给另一个 generator）

}  // namespace

int main() {
    log("main", "─── A-1：第一个 generator ───");

    // ── 必做 2/3/4：消费 fibonacci(10) 的前 10 项 ─────────────────────
    log("main", "构造 gen = fibonacci(10) —— 此刻协程体应该一行都没执行");
    auto gen = fibonacci(10);
    log("main", "构造完毕，准备进入 for 循环 —— 第一次 ++ 才会触发 first resume");

    int idx = 0;
    for (int v : gen) {
        log("main", "got [", idx++, "] = ", v);
    }

    // ── 必做 6：消费 read_lines ───────────────────────────────────────
    log("main", "─── read_lines ───");
    int line_no = 0;
    for (auto const& line : read_lines()) {
        log("main", "line ", line_no++, ": ", line);
    }

    // ── 进阶：用 views::take 截取无限 generator ──────────────────────
    // TODO [进阶]:
    //   for (int v : std::views::take(fibonacci_inf(), 15)) { ... }
    //   注意：fibonacci_inf() 是一个会被立即销毁的临时 generator，
    //   range-based for 会延长其生命周期到循环结束（标准已保证）。

    return 0;
}
