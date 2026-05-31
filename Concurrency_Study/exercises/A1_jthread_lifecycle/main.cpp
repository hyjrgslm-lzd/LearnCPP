// =====================================================================
// 练习 A1_jthread_lifecycle：jthread 生命周期与自动 join
//   对应文档：Concurrency_Study/02-模块A-线程生命周期与jthread.md（练习 A-1）
//
//   官方参考：
//     - std::jthread : https://en.cppreference.com/w/cpp/thread/jthread
//     - std::thread  : https://en.cppreference.com/w/cpp/thread/thread
//     - std::thread::~thread（未 join/detach 调用 std::terminate）：
//         https://en.cppreference.com/w/cpp/thread/thread/~thread
//
//   学习目标：
//     1. 区分 std::thread（C++11）与 std::jthread（C++20）的生命周期语义。
//     2. 亲手观察 jthread 离开作用域时自动 request_stop() + join()。
//     3. 理解“未 join/detach 的 std::thread 析构会调用 std::terminate”这条铁律，
//        并用一种【不真的终止本进程】的方式把它演示清楚。
//
//   术语：可结合线程（joinable thread）、协作式取消（cooperative cancellation）、
//         资源获取即初始化（RAII，Resource Acquisition Is Initialization）。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <thread>      // std::thread, std::jthread, std::this_thread
#include <stop_token>  // std::stop_token（C++20，jthread 自带）

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// 演示 1：std::thread 必须手动 join，否则析构即 terminate。
// 这里我们【正确地】join，避免触发 terminate。下方 demo_thread_forgot_join()
// 用注释解释“如果忘记 join 会怎样”，不真的让它发生。
// ---------------------------------------------------------------------
void demo_thread_manual_join() {
    cs::println("---- 演示 1：std::thread 手动 join ----");

    std::thread worker([] {
        cs::log("[thread] worker 开始干活");
        std::this_thread::sleep_for(100ms);
        cs::log("[thread] worker 干完了");
    });

    // TODO [必做 1]: 在这里手动 join 这个 std::thread。
    //   真正该做的：调用 worker.join()，阻塞当前线程直到 worker 跑完。
    //   下面是最小占位实现，保证编译通过；请替换为真正的 join。
    //   参考实现：
    //       worker.join();
    if (worker.joinable()) {
        worker.join();  // 占位：直接 join，避免 terminate。真做时把上面的参考实现写出来即可。
    }

    cs::log("[thread] 主线程：worker 已 join，作用域安全退出");
}

// ---------------------------------------------------------------------
// 演示 1b：“忘记 join 的 std::thread 会 terminate” —— 只说明，不真的执行。
// 我们把会触发 terminate 的代码放进一个【从不被调用】的函数里，并配文字说明，
// 这样既讲清楚了机制，又不会把整套测试驱动给 abort 掉。
// ---------------------------------------------------------------------
void demo_thread_forgot_join_DO_NOT_CALL() {
    // 下面这段如果真的执行：worker 是 joinable 的 std::thread，
    // 函数返回时 worker 析构，~thread() 发现仍 joinable，于是调用 std::terminate()，
    // 整个进程 abort。这就是 C++ 设计者的刻意选择：“线程泄漏”是程序错误，宁可崩也不静默。
    //
    //   std::thread worker([]{ std::this_thread::sleep_for(1s); });
    //   // 既不 join 也不 detach……
    //   return;  // <-- 这里 ~thread() 触发 std::terminate()
    //
    // 把它写进【永不调用】的函数里，是为了让你读到机制而不真的崩溃。
    cs::log("[占位] demo_thread_forgot_join_DO_NOT_CALL 不应被调用");
}

// ---------------------------------------------------------------------
// 演示 2：std::jthread 离开作用域自动 join（RAII）。
// ---------------------------------------------------------------------
void demo_jthread_auto_join() {
    cs::println("---- 演示 2：std::jthread 自动 join ----");

    {
        std::jthread worker([] {
            cs::log("[jthread] worker 开始干活");
            std::this_thread::sleep_for(100ms);
            cs::log("[jthread] worker 干完了");
        });

        cs::log("[jthread] 主线程：worker 已启动，即将离开内层作用域");
        // TODO [必做 2]: 这里【不要】手动调用 join。
        //   真正要观察的：当 worker 离开这个内层作用域时，~jthread() 会自动
        //   先 request_stop()（请求停止），再 join()（等待结束）。
        //   你要做的只是“什么都不写”，然后从日志时间戳确认：
        //   下面那行 “内层作用域结束” 一定出现在 worker “干完了” 之后。
    } // <-- worker 在此析构：自动 request_stop() + join()

    cs::log("[jthread] 主线程：内层作用域结束（worker 必已 join 完毕）");
}

// ---------------------------------------------------------------------
// 演示 3：jthread 析构会先 request_stop()，配合循环型 worker 可优雅退出。
// （A2 会专门深入协作式取消；这里只先建立“自动 request_stop”的直觉。）
// ---------------------------------------------------------------------
void demo_jthread_auto_request_stop() {
    cs::println("---- 演示 3：jthread 析构自动 request_stop ----");

    {
        // jthread 的可调用对象若把 std::stop_token 作为第一个形参，
        // 运行期会自动把该 jthread 内置的 stop_token 传进来。
        std::jthread worker([](std::stop_token st) {
            int loops = 0;
            // TODO [进阶 1]: 把循环条件改成检查协作式取消信号。
            //   真正该写：while (!st.stop_requested()) { ...干活... }
            //   这样当外层 jthread 析构调用 request_stop() 时，stop_requested()
            //   变 true，循环自然退出，worker 优雅结束。
            //   下面是最小占位（固定跑 5 次就退出），保证编译通过；
            //   请替换为基于 st 的协作式循环。
            //   参考实现：
            //       while (!st.stop_requested()) {
            //           cs::logf("[jthread/loop] tick ", loops++);
            //           std::this_thread::sleep_for(30ms);
            //       }
            (void)st;  // 占位期间避免“未使用形参”告警
            while (loops < 5) {
                cs::logf("[jthread/loop] tick ", loops++);
                std::this_thread::sleep_for(30ms);
            }
            cs::log("[jthread/loop] worker 退出循环");
        });

        std::this_thread::sleep_for(120ms);
        cs::log("[jthread/loop] 主线程：即将离开作用域，jthread 将自动 request_stop");
    } // <-- worker 析构：先 request_stop()，再 join()

    cs::log("[jthread/loop] 主线程：worker 已优雅结束");
}

int main() {
    cs::println("===== 练习 A-1：jthread 生命周期与自动 join =====");

    demo_thread_manual_join();
    demo_jthread_auto_join();
    demo_jthread_auto_request_stop();

    cs::println("===== A-1 测试驱动结束 =====");
    // 验收（看 stdout 即可，无需第三方测试框架）：
    //   - 演示 1：worker 的“干完了”必在“已 join”之前。
    //   - 演示 2：jthread 的“干完了”必在“内层作用域结束”之前（证明自动 join）。
    //   - 演示 3：填好进阶 1 后，worker 在主线程“即将离开作用域”后不久退出循环
    //            （证明析构自动 request_stop 触发了协作式退出）。
    return 0;
}
