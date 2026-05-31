// =====================================================================
// 练习 D-1：promise / future 基础（promise / future / shared_future）
//   对应文档：Concurrency_Study/05-模块D-future与异步任务.md 的 练习 D-1
//
//   学习目标：
//     - 掌握 std::promise（写端）/ std::future（读端）构成的
//       一次性通道（one-shot channel）：set_value / set_exception / get；
//     - 理解 future.get() 为什么只能取一次、值如何被“取走”；
//     - 用 set_exception 把异常跨线程原样传回，在 get() 处捕获；
//     - 用 std::shared_future 让多个消费者等同一个结果、各自多次 get；
//     - 认识 broken_promise：生产方不设值就析构的安全网。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/promise
//     - https://en.cppreference.com/w/cpp/thread/future
//     - https://en.cppreference.com/w/cpp/thread/shared_future
//     - https://en.cppreference.com/w/cpp/thread/future_error
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章 4.2
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target D1_promise_future --config Release
//     ./build-vs2026/D1_promise_future/Release/D1_promise_future.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <chrono>
#include <exception>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

// =====================================================================
// 必做 1：promise 传值。
//   worker 按值接收 promise（move 进来），算出结果后 set_value；
//   主线程握着配对的 future，get() 阻塞到就绪并取出。
// =====================================================================
void worker_set_value(std::promise<int> p) {
    cs::logf("[value] worker 启动，开始计算…");
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // 模拟耗时计算

    // TODO [必做 1]: 把计算结果通过 promise 送回主线程。
    //   要点：promise 是写端，set_value 把结果写入共享状态，
    //   主线程那一侧配对的 future.get() 随即可取出。
    //   下面已是正确写法，留作必做 1 的参考实现：
    int result = 21 * 2;
    p.set_value(result);
    cs::logf("[value] worker 已 set_value(", result, ")。");
}

void demo_promise_value() {
    cs::println("================ 必做 1：promise 传值 ================");

    std::promise<int> p;
    std::future<int> fut = p.get_future(); // 取出配对的读端

    // promise 不可拷贝：必须 move 进 worker 线程。
    std::thread t(worker_set_value, std::move(p));

    cs::logf("[value] 主线程在 future.get() 上等待结果…");
    int v = fut.get(); // 阻塞直到就绪；get 之后 fut 失效
    cs::logf("[value] 主线程取到结果 = ", v, "（fut.valid()=", fut.valid(), "）");

    t.join();
    cs::println("");
}

// =====================================================================
// 必做 2：promise 传异常。
//   worker 计算中“出错”，用 set_exception 把异常送过通道；
//   主线程 future.get() 时该异常被原样重新抛出，在 try/catch 接住。
// =====================================================================
void worker_set_exception(std::promise<int> p) {
    cs::logf("[exc] worker 启动，计算中将抛出异常…");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // TODO [必做 2]: 用 set_exception 把异常送过一次性通道。
    //   要点：异常不是此刻抛出，而是被“存进”通道，
    //   推迟到消费方 get() 时才重新抛出（与返回值在 get 时取出对称）。
    //   下面已是正确写法，留作必做 2 的参考实现：
    try {
        throw std::runtime_error("worker 计算失败：boom");
    } catch (...) {
        p.set_exception(std::current_exception());
        cs::logf("[exc] worker 已 set_exception（异常已存入通道）。");
    }
    // 备选等价写法（不经 try）：
    //   p.set_exception(std::make_exception_ptr(std::runtime_error("boom")));
}

void demo_promise_exception() {
    cs::println("============== 必做 2：promise 传异常 ==============");

    std::promise<int> p;
    std::future<int> fut = p.get_future();
    std::thread t(worker_set_exception, std::move(p));

    cs::logf("[exc] 主线程 future.get() 等待（预期会重新抛出异常）…");
    try {
        int v = fut.get();
        cs::logf("[exc] 不应到达此处，v=", v);
    } catch (const std::exception& e) {
        cs::logf("[exc] 主线程在 get() 处捕获到跨线程传回的异常：", e.what());
    }

    t.join();
    cs::println("");
}

// =====================================================================
// 必做 3：shared_future 多消费者。
//   普通 future 取一次即失效；share() 转成 shared_future 后可拷贝、
//   可被多个线程各自 get() 多次，且每次拿到同一个结果。
// =====================================================================
void demo_shared_future() {
    cs::println("============ 必做 3：shared_future 多消费者 ============");

    std::promise<int> p;
    std::future<int> fut = p.get_future();

    // TODO [必做 3]: 把 future 转成 shared_future，分发给多个等待者。
    //   要点：share() 后原 fut 失效，所有权交给可拷贝的 shared_future；
    //   每个等待线程按值拷贝一份 sf，各自 get() 都拿到同一结果。
    //   下面已是正确写法，留作必做 3 的参考实现：
    std::shared_future<int> sf = fut.share();
    cs::logf("[shared] share() 之后原 fut.valid()=", fut.valid(),
             "，sf.valid()=", sf.valid());

    constexpr int kConsumers = 3;
    std::vector<std::thread> ts;
    for (int i = 0; i < kConsumers; ++i) {
        ts.emplace_back([sf, i] {            // 按值拷贝 shared_future（合法）
            int v = sf.get();                // 多个消费者各自 get 同一结果
            cs::logf("[shared] 消费者 ", i, " 取到结果 = ", v);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(120)); // 让消费者先就位
    cs::logf("[shared] 生产方 set_value(100)，广播给全部消费者。");
    p.set_value(100);

    for (auto& t : ts) t.join();
    cs::println("");
}

// =====================================================================
// 进阶 1：broken_promise 安全网。
//   promise 不设值就析构 → 共享状态被置为“坏掉”，future.get()
//   抛 std::future_error，错误码 broken_promise。这是“生产方没履约”
//   的兜底信号，与 worker 主动 set_exception 传回的业务异常不同。
// =====================================================================
void demo_broken_promise() {
    cs::println("============== 进阶 1：broken_promise 安全网 ==============");

    std::future<int> fut;
    {
        std::promise<int> p;
        fut = p.get_future();
        cs::logf("[broken] promise 即将不设值就离开作用域…");
        // TODO [进阶 1]: 故意不调用 set_value / set_exception，
        //   让 p 在此处析构，触发 broken_promise。
        //   （此处什么都不做即可——p 出作用域被销毁。）
    } // p 析构：共享状态变“坏”

    try {
        int v = fut.get();
        cs::logf("[broken] 不应到达此处，v=", v);
    } catch (const std::future_error& e) {
        cs::logf("[broken] 捕获 std::future_error：", e.what(),
                 "（code == broken_promise? ",
                 (e.code() == std::future_errc::broken_promise), "）");
    }
    cs::println("");
}

int main() {
    cs::println("==== D1_promise_future：promise / future / shared_future ====\n");

    demo_promise_value();      // 必做 1：promise 传值
    demo_promise_exception();  // 必做 2：promise 传异常
    demo_shared_future();      // 必做 3：shared_future 多消费者
    demo_broken_promise();     // 进阶 1：broken_promise 安全网

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
