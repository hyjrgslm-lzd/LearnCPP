// =====================================================================
// 练习 A2_stop_token_cancellation：stop_token 协作式取消
//   对应文档：Concurrency_Study/02-模块A-线程生命周期与jthread.md（练习 A-2）
//
//   官方参考：
//     - std::stop_token    : https://en.cppreference.com/w/cpp/thread/stop_token
//     - std::stop_source   : https://en.cppreference.com/w/cpp/thread/stop_source
//     - std::stop_callback : https://en.cppreference.com/w/cpp/thread/stop_callback
//     - std::jthread::request_stop :
//         https://en.cppreference.com/w/cpp/thread/jthread/request_stop
//
//   学习目标：
//     1. 理解“取消是协作式（cooperative cancellation）的”：没有人能强行 kill
//        一个 std::thread/jthread，只能【请求】它停，由它自己在检查点退出。
//     2. 写一个轮询 stop_token 的工作循环，并用 request_stop() 触发它退出。
//     3. 用 std::stop_callback 注册一个“取消发生时立即执行”的回调，记录取消时刻。
//
//   核心心智模型：std::stop_source 是“开关”，std::stop_token 是“开关的只读视图”，
//     std::stop_callback 是“开关被按下瞬间触发的铃”。jthread 内部自带一个
//     stop_source，可用 get_stop_token() 取到对应 token。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <thread>
#include <stop_token>  // std::stop_token / std::stop_source / std::stop_callback（C++20）

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// 演示 1：jthread 自带 stop_token —— worker 轮询，主线程 request_stop。
// ---------------------------------------------------------------------
void demo_jthread_polling_cancel() {
    cs::println("---- 演示 1：jthread 轮询 stop_token 并被取消 ----");

    std::jthread worker([](std::stop_token st) {
        long long tick = 0;
        // TODO [必做 1]: 写一个轮询协作式取消信号的工作循环。
        //   真正该写：每轮干一点活，然后检查 st.stop_requested()，为 true 就退出。
        //   下面是最小占位（固定 8 轮就退出），保证编译通过；
        //   请替换为基于 st 的循环，这样它才会“被取消”而不是“跑够次数”。
        //   参考实现：
        //       while (!st.stop_requested()) {
        //           cs::logf("[worker] 第 ", tick++, " 轮：处理一批数据");
        //           std::this_thread::sleep_for(50ms);
        //       }
        (void)st;
        while (tick < 8) {
            cs::logf("[worker] 第 ", tick++, " 轮：处理一批数据");
            std::this_thread::sleep_for(50ms);
        }
        cs::log("[worker] 检测到取消请求，干净退出循环");
    });

    // 让 worker 跑一会儿，再请求它停下。
    std::this_thread::sleep_for(180ms);
    cs::log("[main] 发出取消请求 request_stop()");

    // TODO [必做 2]: 通过 jthread 句柄发出取消请求。
    //   真正该写：worker.request_stop();
    //   它会把 jthread 内置 stop_source 翻成“已请求停止”，使 worker 端
    //   st.stop_requested() 返回 true。下面是占位（什么也不做时，必做1的占位
    //   循环会自己跑完）；请替换为真正的 request_stop()。
    //   参考实现：
    //       worker.request_stop();
    // worker.request_stop();   // <-- 取消注释并删除占位说明即可

    // worker 析构时也会自动 request_stop()+join()，所以这里不显式 join。
    cs::log("[main] 等待 worker 结束（jthread 析构会自动 join）");
}

// ---------------------------------------------------------------------
// 演示 2：用独立的 stop_source/stop_token 控制一个普通 std::thread。
// 这说明协作式取消机制【不绑定】jthread，stop_source 可独立存在并共享给多个线程。
// ---------------------------------------------------------------------
void demo_external_stop_source() {
    cs::println("---- 演示 2：外部 stop_source 控制 std::thread ----");

    std::stop_source source;                       // 开关
    std::stop_token token = source.get_token();    // 只读视图，传给 worker

    std::thread worker([token] {                   // 按值捕获 token（可廉价拷贝）
        long long tick = 0;
        while (!token.stop_requested()) {          // 此处直接示范正确写法
            cs::logf("[ext-worker] 第 ", tick++, " 轮");
            std::this_thread::sleep_for(50ms);
        }
        cs::log("[ext-worker] 收到取消，退出");
    });

    std::this_thread::sleep_for(180ms);
    cs::log("[main] source.request_stop()");
    source.request_stop();                         // 拨动开关：所有持有该 token 的线程都会看到

    worker.join();                                 // 普通 thread：必须手动 join
    cs::log("[main] ext-worker 已 join");
}

// ---------------------------------------------------------------------
// 演示 3：stop_callback —— 取消发生的瞬间触发回调，记录“取消时刻”。
// ---------------------------------------------------------------------
void demo_stop_callback() {
    cs::println("---- 演示 3：stop_callback 记录取消时刻 ----");

    std::jthread worker([](std::stop_token st) {
        // TODO [进阶 1]: 在 worker 内注册一个 std::stop_callback。
        //   真正该做：构造一个 std::stop_callback，绑定到 st 上，
        //   回调体内打印“取消在此刻被请求”。语义要点：
        //     - 若注册时 st 已被请求取消，回调会在【构造该 stop_callback 的线程】上
        //       同步立即执行；
        //     - 否则回调会在【调用 request_stop 的那个线程】上执行。
        //   下面是占位（不注册回调，仅轮询）；请补上 stop_callback。
        //   参考实现：
        //       std::stop_callback cb{st, [] {
        //           cs::log("[callback] 取消被请求 —— 在此刻触发！");
        //       }};
        long long tick = 0;
        while (!st.stop_requested()) {
            cs::logf("[cb-worker] 第 ", tick++, " 轮");
            std::this_thread::sleep_for(50ms);
        }
        cs::log("[cb-worker] 退出循环");
    });

    std::this_thread::sleep_for(170ms);
    cs::log("[main] 即将 request_stop（回调应在此后几乎立刻触发）");
    worker.request_stop();   // 这里显式请求，便于观察回调与请求的先后

    // worker 析构自动 join。
    cs::log("[main] 已请求取消，等待 worker 结束");
}

int main() {
    cs::println("===== 练习 A-2：stop_token 协作式取消 =====");

    demo_jthread_polling_cancel();
    demo_external_stop_source();
    demo_stop_callback();

    cs::println("===== A-2 测试驱动结束 =====");
    // 验收（看 stdout）：
    //   - 演示 1（填好必做 1/2 后）：worker 在 main 发出 request_stop 后“干净退出循环”，
    //     而不是跑满固定次数。
    //   - 演示 2：source.request_stop() 后 ext-worker 很快退出并被 join。
    //   - 演示 3（填好进阶 1 后）：[callback] 一行出现在 main “即将 request_stop” 之后、
    //     cb-worker 下一轮之前，证明回调在取消瞬间触发。
    return 0;
}
