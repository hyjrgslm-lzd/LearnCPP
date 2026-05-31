// I-4 Boost.Cobalt channel + race + gather
// 文档参考：11-模块I-真实异步IO与并发框架.md 「练习 I-4」
// 官方参考：
//   - https://www.boost.org/doc/libs/master/libs/cobalt/doc/html/index.html
//   - boost/cobalt/channel.hpp / race.hpp / gather.hpp / main.hpp
//   - Klemens Morgenstern "Coroutines and Channels" CppCon 2023
//
// 目标：用 Boost.Cobalt 的 channel<T> + race + gather 搭建生产者-消费者管道。
//      展示：channel 的 symmetric transfer 通信、race 的"任一完成取消其余"语义、
//      gather 的全量汇合、以及单线程执行器下的背压死锁陷阱。
//
// 关键约定：cobalt::main 不是返回类型；它通过宏 BOOST_COBALT_MAIN 把 co_main(...)
//          包装为协程入口，宏内部创建 io_context 并驱动协程。

#include <boost/cobalt.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/race.hpp>
#include <boost/cobalt/gather.hpp>

#include <chrono>
#include <iostream>

namespace cobalt = boost::cobalt;

// ============ 生产者 / 消费者 ============
cobalt::task<void> producer(cobalt::channel<int>& ch, int N)
{
    for (int i = 0; i < N; ++i) {
        std::cout << "[prod] sending " << i << std::endl;
        co_await ch.write(i);    // channel 满时挂起；symmetric transfer 到 reader（若存在）
    }
    ch.close();
    co_return;
}

cobalt::task<void> consumer(cobalt::channel<int>& ch)
{
    while (ch.is_open()) {
        auto r = co_await ch.read();
        if (!r) {
            std::cout << "[cons] channel closed" << std::endl;
            break;
        }
        std::cout << "[cons] received " << *r << std::endl;
        co_await cobalt::wait_for(std::chrono::milliseconds(5));
    }
    co_return;
}

// ============ race：用 sleep 给 read 套超时 ============
cobalt::task<void> consumer_with_timeout(cobalt::channel<int>& ch)
{
    while (ch.is_open()) {
        auto result = co_await cobalt::race(
            ch.read(),
            cobalt::wait_for(std::chrono::milliseconds(500))
        );
        if (result.index() == 0) {
            auto opt = std::get<0>(result);
            if (!opt) { std::cout << "[cons-t] closed" << std::endl; break; }
            std::cout << "[cons-t] got " << *opt << std::endl;
        } else {
            std::cout << "[cons-t] timeout" << std::endl;
        }
    }
    co_return;
}

// ============ gather：并发等待多个 task 全部完成 ============
cobalt::task<void> demo_gather()
{
    cobalt::channel<int> ch{4};
    co_await cobalt::gather(producer(ch, 5), consumer(ch));
    co_return;
}

// ============ co_main 入口（由 BOOST_COBALT_MAIN 宏注入） ============
// 宏会展开为：int main() { boost::asio::io_context; co_main(...); ... }
cobalt::main co_main(int argc, char* argv[])
{
    std::cout << "===== I-4: Boost.Cobalt channel + race + gather =====\n\n";

    {
        std::cout << "--- 测试 1：基本 producer/consumer 管道 ---\n";
        cobalt::channel<int> ch{2};      // 容量 2
        co_await cobalt::gather(producer(ch, 5), consumer(ch));
    }

    {
        std::cout << "\n--- 测试 2：race 实现 read 超时 ---\n";
        cobalt::channel<int> ch{2};
        auto p = [&]() -> cobalt::task<void> {
            co_await cobalt::wait_for(std::chrono::seconds(1));
            co_await ch.write(99);
            ch.close();
            co_return;
        };
        co_await cobalt::gather(p(), consumer_with_timeout(ch));
    }

    // TODO [必做]：把 channel 容量改成 1，观察 producer 写第 2 个时是否挂起。
    // TODO [必做]：在笔记中分析单线程执行器下何时不会死锁——
    //   生产者挂起即释放执行器，消费者得以被调度。
    // TODO [进阶]：multi-consumer channel——多个 reader 公平争夺数据。
    // TODO [进阶]：3-stage pipeline，stage 间用 channel 串联。
    // TODO [进阶]：用 race 实现"channel 写入超时"。

    std::cout << "\n===== Done =====\n";
    co_return 0;
}
