// =====================================================================
// 练习 A3_thread_exception：线程中的异常传播
//   对应文档：Concurrency_Study/02-模块A-线程生命周期与jthread.md（练习 A-3）
//
//   官方参考：
//     - std::exception_ptr     : https://en.cppreference.com/w/cpp/error/exception_ptr
//     - std::current_exception : https://en.cppreference.com/w/cpp/error/current_exception
//     - std::rethrow_exception : https://en.cppreference.com/w/cpp/error/rethrow_exception
//     - std::promise::set_exception :
//         https://en.cppreference.com/w/cpp/thread/promise/set_exception
//
//   学习目标：
//     1. 记住铁律：线程函数（thread/jthread 的入口）若让异常逃逸出顶层，
//        会调用 std::terminate()，整个进程 abort —— 异常【不会】自动跨线程传播。
//     2. 学会用 std::exception_ptr + std::current_exception/std::rethrow_exception
//        在 worker 里“捕获并打包”异常，在主线程“解包并重抛”后处理。
//     3. 学会用 std::promise::set_exception 走 future/promise 通道传播异常
//        （这条路 D 模块还会深入；此处先建立直觉）。
//
//   术语：异常指针（exception_ptr）、当前异常（current_exception）、
//         重新抛出（rethrow_exception）。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <exception>   // std::exception_ptr / current_exception / rethrow_exception
#include <future>      // std::promise / std::future（演示 2）
#include <stdexcept>   // std::runtime_error
#include <thread>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// 演示 0：“异常逃逸线程顶层 = std::terminate” —— 只说明，不真的执行。
// 放进【永不调用】的函数，避免把测试驱动 abort 掉。
// ---------------------------------------------------------------------
void demo_uncaught_in_thread_DO_NOT_CALL() {
    // 如果真的执行下面这段：lambda 在子线程里抛出异常且无人捕获，
    // 异常到达线程顶层 -> std::terminate() -> 进程 abort。
    // 注意：try/catch 写在 main 里【捕不到】子线程的异常，因为它们不在同一调用栈。
    //
    //   std::jthread t([]{ throw std::runtime_error("boom from thread"); });
    //   // 无论主线程怎么 try，这个 boom 都会让进程 terminate。
    //
    cs::log("[占位] demo_uncaught_in_thread_DO_NOT_CALL 不应被调用");
}

// ---------------------------------------------------------------------
// 演示 1：exception_ptr 手动跨线程传播。
// worker 在自己的 try/catch 里捕获异常，用 current_exception() 打包成
// exception_ptr 存到共享变量；主线程 join 后 rethrow 并处理。
// ---------------------------------------------------------------------
void demo_exception_ptr() {
    cs::println("---- 演示 1：exception_ptr 手动传播 ----");

    std::exception_ptr captured;   // 跨线程“信箱”：worker 写，主线程读（join 建立 happens-before）

    std::jthread worker([&captured] {
        try {
            cs::log("[worker] 开始干活，即将抛出异常");
            std::this_thread::sleep_for(50ms);
            throw std::runtime_error("worker 算到一半失败了");
        } catch (...) {
            // TODO [必做 1]: 在 worker 的 catch 里把“当前异常”打包进 captured。
            //   真正该写：captured = std::current_exception();
            //   current_exception() 返回一个指向当前正在处理异常的 exception_ptr，
            //   它可以安全地跨线程传递（引用计数管理）。
            //   下面是占位（不打包，留空），保证编译通过；请替换为参考实现。
            //   参考实现：
            //       captured = std::current_exception();
            cs::log("[worker] 捕获到异常（占位：尚未打包，请补 TODO 必做 1）");
        }
    });

    // jthread 在此析构时自动 join；为了在 join 之后再读 captured，
    // 我们手动 join 一次（join 后再 join 是安全的：joinable 变 false 就不重复）。
    worker.join();
    cs::log("[main] worker 已 join，检查信箱");

    // TODO [必做 2]: 在主线程检查并重抛 captured，然后 catch 处理。
    //   真正该写：
    //       if (captured) {
    //           try { std::rethrow_exception(captured); }
    //           catch (const std::exception& e) {
    //               cs::logf("[main] 跨线程捕获到异常：", e.what());
    //           }
    //       }
    //   下面是占位：仅判断信箱是否为空。补完 TODO 必做 1 后，captured 才非空，
    //   这里也要替换为真正的 rethrow + catch。
    if (captured) {
        cs::log("[main] 信箱非空（占位：请补 TODO 必做 2 做 rethrow + 处理）");
        // 参考实现见上方注释。
    } else {
        cs::log("[main] 信箱为空（说明 TODO 必做 1 还没填，worker 没打包异常）");
    }
}

// ---------------------------------------------------------------------
// 演示 2：promise/future 通道传播异常。
// worker 把异常塞进 promise，主线程 future.get() 时异常被自动重抛。
// 这是“正式”的跨线程错误传播通道（D 模块详讲）。
// ---------------------------------------------------------------------
void demo_promise_set_exception() {
    cs::println("---- 演示 2：promise::set_exception 传播异常 ----");

    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    std::jthread worker([p = std::move(prom)]() mutable {
        try {
            cs::log("[worker2] 计算中……即将失败");
            std::this_thread::sleep_for(50ms);
            throw std::runtime_error("worker2 计算异常");
            // 正常路径下应是：p.set_value(42);
        } catch (...) {
            // TODO [进阶 1]: 把当前异常通过 promise 传出去。
            //   真正该写：p.set_exception(std::current_exception());
            //   这样主线程 fut.get() 会重新抛出这个异常。
            //   下面是占位：为避免 future 永久阻塞（promise 析构未设值会让 get()
            //   抛 broken_promise），这里先 set_value 一个哨兵值。
            //   补好进阶 1 后，请删掉占位的 set_value，改用 set_exception。
            //   参考实现：
            //       p.set_exception(std::current_exception());
            cs::log("[worker2] 捕获异常（占位：用 set_value 哨兵，请改为 set_exception）");
            p.set_value(-1);  // 占位哨兵；补 TODO 后删除并改 set_exception
        }
    });

    // TODO [进阶 2]: 在主线程 get() 结果，并用 try/catch 接住可能的异常。
    //   真正该写：
    //       try {
    //           int v = fut.get();           // 若 worker set_exception，这里会重抛
    //           cs::logf("[main2] 拿到结果：", v);
    //       } catch (const std::exception& e) {
    //           cs::logf("[main2] future.get() 重抛异常：", e.what());
    //       }
    //   下面是占位：直接 get 哨兵值。补好进阶 1/2 后替换为上面的 try/catch。
    int v = fut.get();
    cs::logf("[main2] future.get() 返回（占位）：", v,
             "  —— 补好进阶 1/2 后这里应捕获到异常");
    // worker 析构自动 join。
}

int main() {
    cs::println("===== 练习 A-3：线程中的异常传播 =====");

    demo_exception_ptr();
    demo_promise_set_exception();

    cs::println("===== A-3 测试驱动结束 =====");
    // 验收（看 stdout）：
    //   - 演示 1（填好必做 1/2 后）：[main] 一行打印出“跨线程捕获到异常：worker 算到一半失败了”。
    //   - 演示 2（填好进阶 1/2 后）：[main2] 一行打印出“future.get() 重抛异常：worker2 计算异常”，
    //     而不是返回哨兵值 -1。
    //   - 演示 0 永远不调用：证明你理解“异常逃逸线程顶层会 terminate”而无需真的崩溃。
    return 0;
}
