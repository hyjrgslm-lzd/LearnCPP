# I-4 Boost.Cobalt channel + race + gather

对应文档：`11-模块I-真实异步IO与并发框架.md` 「练习 I-4」。

## Windows 默认跳过

Boost.Cobalt 在 Windows 上配置复杂（依赖完整 Boost + Asio + MSVC ABI 匹配），
CMakeLists 顶部 `if(WIN32) return() endif()`，仅在 Linux/macOS 真正构建。

## 目标

用 Boost.Cobalt 的 `channel<T>` + `race` + `gather` 搭建生产者-消费者管道，理解：
- `channel.write/read` 的 symmetric transfer 通信——写者和读者在同执行器上时，
  `await_suspend` 直接返回对方的 `coroutine_handle`，零调度开销。
- `race` 的"任一完成取消其余"语义——天然适合超时模式。
- `gather` 的全量汇合——并发等待多个 task 都完成。
- 单线程执行器下的背压死锁陷阱——以及为什么"挂起即释放执行器"通常能避免它。

## cobalt::main 入口约定

Boost.Cobalt 当前 API 用宏 `BOOST_COBALT_MAIN` 把 `co_main(...)` 包装为 main：
```cpp
cobalt::main co_main(int argc, char* argv[]) { ... co_return 0; }
```
不是返回类型——是宏式入口。骨架已经按这个写法布置。

## 必做任务

1. 编译运行：观察基本 producer/consumer 输出，5 个数全部送达。
2. 把 channel 容量改成 1，观察生产者写第 2 个时是否挂起。
3. 跑测试 2：race 给 read 套 500ms 超时，前 1 秒内会打印多次 timeout，
   1 秒后才收到 99。
4. 在笔记中分析单线程执行器下背压死锁的真实条件——
   "生产者挂起即释放执行器"为什么通常能避免死锁？

## 进阶任务

- multi-consumer channel：多个 reader 公平争夺数据。
- 3-stage pipeline：stage 间用 channel 串联，测吞吐与延迟。
- 用 race 实现"channel 写入超时"：超时则放弃写入。

## 验收点

- 基本管道能正确传输 N 个数。
- race 给 read 套超时能正确轮替"timeout"和"got"。
- 你能讲清 channel 的 symmetric transfer 优化为什么在单执行器场景下零开销。
- 你能讲清"挂起即释放执行器"为何让大多数 channel 用法不死锁。

## 提示

- `co_await ch.write(v)` 在 channel 满时挂起——这本身就释放了执行器。
- `cobalt::race` 的结果是 `std::variant`，用 `.index()` 或 `std::get_if` 判断哪个分支胜出。
- 如果 `<boost/cobalt.hpp>` 找不到，确认 vcpkg/Conan 正确安装了 `boost-cobalt`。
