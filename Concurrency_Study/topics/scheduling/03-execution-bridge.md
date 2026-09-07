# 调度 03：把值、错误和停止组合成一条执行管线

M1 中每次 submit 返回一个 future。两个任务完成后相加，需要显式保存两个 future，分别 get，再处理异常和执行资源寿命。这种写法适合少量任务；当工作分成多个异步阶段，结果之间的连接会散落在控制流程中。sender/receiver 将这些连接表示成可组合的操作描述，让“得到什么结果”和“在哪里执行”能在同一条管线中表达。

本篇运行 [M2 main.cpp](../../exercises/M2_execution_bridge/main.cpp) 与 [solution.cpp](../../exercises/M2_execution_bridge/solution.cpp)，使用 NVIDIA stdexec 固定 `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`，对应 nvhpc-26.05 发布线。标准说明固定对照 [C++26 工作草案 N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)；P2300 是历史提案，滚动 eel 页面可能已包含 C++29 改动，不能把它们当成这个固定实现的逐字接口说明。

## 1. 四个角色，先避免把它们都叫“线程”

`exec::static_thread_pool` 拥有真实执行资源，提供 scheduler。scheduler 是可复制的调度句柄，用于请求某种执行上下文，并不等同于拥有整个池。`schedule(scheduler)` 返回 sender，描述一次调度完成。`then` 把上游成功值变换为下游值。receiver 是操作完成时接收 value、error 或 stopped 的对象。

构造 sender 表达式一般只构造描述；让一次操作发生还需要 connect 生成 operation state，再 start。M2 用 sync_wait 作为 consumer，替调用者持有结果状态、连接并启动操作、等待完成。不要把 `just(10)` 理解成已经起了一条异步线程：它可同步发送一个值，是否切到池取决于管线中的调度操作。

sender 描述与 operation state 也不是同一个对象。已经启动的 operation state 必须保持存活直到相应协议允许销毁；把一个 sender 保存起来，不会自动延长它引用的输入对象或 scheduler 对应资源的寿命。

## 2. 成功路径怎样流动

main 的代码直接可运行：

```cpp
auto pipeline = stdexec::schedule(pool.get_scheduler())
    | stdexec::then([] { return 21; })
    | stdexec::then([](int value) { return value * 2; });
auto result = stdexec::sync_wait(std::move(pipeline));
cs::check(result && std::get<0>(*result) == 42, "value pipeline");
```

schedule 的成功完成不带业务值，第一个 then 因而是无参数函数。它产出 21，第二个 then 接收 int 并产出 42。sync_wait 成功时返回有值的 optional，其内部是完成值构成的 tuple。tuple 即使只有一个元素也不是多余层次，因为 sender 的成功完成可以有多个参数。

先检查 optional 是否有值再取 tuple，是契约的一部分。停止完成不会给出一个默认构造的 0，也不该被当作“计算结果为空所以失败”；它是独立的完成通道。

## 3. when_all 汇合的是完成，不只是数值相加

Reference 构造两个 `schedule | then` 分支，分别产出 100 和 23。`when_all(a,b)` 把它们组合，下游 then 接收两个参数后相加，sync_wait 检查 123。一个原子 completed 计数另行检查两个业务函数都已经执行，避免只看某个打印值。

when_all 启动其子操作并在所有子操作完成后完成，但不保证它们一定在同一时刻并行执行。用一个线程的 scheduler、同步完成的 sender 或受限执行资源时，分支可以串行推进。组合关系不等于硬件并行度保证。

若一个分支报错，其他分支可能收到停止请求；停止是协作请求，不是强行终止业务函数。已经开始运行且忽略停止的函数仍必须自行返回。when_all 的父操作要等待所有子完成才能向外报告最后完成，不能在某个 error 一到时就销毁仍在运行的其他子操作。

这也解释了 Reference 的 error 检查为什么不要求另一个分支的业务函数一定执行：在它开始前，调度阶段就可能响应停止。检查要求的是异常经汇合与 sync_wait 正确传回，而不是强求所有 then 在错误路径都执行一遍。

## 4. 三条完成通道要分别测试

value 测试是 42 与 123。error 测试在池上的 then 中抛出带明确消息的 runtime_error。算法把它转为错误完成，sync_wait 在调用线程重抛，Reference 检查消息对应预期错误。`then` 只处理成功值；error 和 stopped 不会自动变成传给 then 的普通参数。

为了精确测试 stopped，Reference 提供一个很小的 `stopped_int` sender。它声明可能 `set_value(int)` 或 `set_stopped()`，实际 start 时调用后者。它不是用退出码或打印“模拟停止”替代协议，而是真正通过 stdexec 的 connect/start/receiver 通道完成。

为什么还声明一个 int 成功签名？N5050 的 sync_wait 要求可形成单一成功结果类型。即使这次运行只发送 stopped，类型层面仍需能够计算 `optional<tuple<int>>`。这个小 sender 把“静态可能的完成集合”和“运行时选择的一次完成”分开，避免把一个完全没有成功签名的类型当成所有标准实现都必须接受的例子。

Reference 检查 stopped 绕过 then、sync_wait 返回 disengaged optional，以及 when_all 的停止传播。最后用 `upon_stopped` 显式把停止变成成功值 7。这一步是策略改变：调用者决定提供替代值，而不是框架把取消默默当成成功。

## 5. 任务 join 与资源 join 是两个边界

`when_all` 完成意味着所组合子操作都已完成；`sync_wait` 使当前线程等这个组合完成。它们没有关闭整个线程池。池可能还服务其他操作，scheduler 也只是一个资源句柄。

M2 先构造 pool，再构造并消费所有 sender，最后在同一作用域退出时销毁 pool。每个 sync_wait 在离开前已经完成它启动的操作，因此引用到的局部对象仍然有效。这个顺序构成结构化使用方式，但不能据此宣称“用了 sender 就不会悬空”：把按引用捕获的局部变量提前销毁、让 scheduler 活过资源，仍然是生命周期错误。

同样，不应在固定池 worker 中随意调用阻塞的 sync_wait，等待同池尚未执行的工作。M1 的资源饥饿问题仍可能出现。标准 consumer 的进展委托规则和具体 scheduler 的能力需要匹配，不能把 `stdexec::sync_wait` 当成对任意第三方线程池的自动 help。这里所有阻塞 consumer 都在外部 main 上调用。

## 6. 标准名与实现名必须分开

| 本题实际写法 | N5050 标准层的对应概念 |
|---|---|
| `<stdexec/execution.hpp>` | `<execution>` 中的 sender/receiver 设施 |
| `stdexec::schedule/then/when_all` | `std::execution::schedule/then/when_all` |
| `stdexec::sync_wait` | `std::this_thread::sync_wait`，不是 `std::execution::sync_wait` |
| `exec::static_thread_pool` | 本题使用的库执行资源，不是保证存在的同名标准池 |

这张表表示阅读映射，不保证简单替换命名空间就能迁移。N5050 与 nvhpc-26.05 在能力、约束和扩展上可能不同；自定义 sender 的具体写法也应按所用版本验证。课程不根据年份推断标准库支持，而使用主线程配置出的能力宏；本机 stdexec 分支可用，原生标准 sender 分支未通过能力检测。

## 7. 构建、观察与边界

在 `Concurrency_Study/exercises` 的已下载依赖环境中，可以配置独立叶项目：

```powershell
cmake -S M2_execution_bridge -B build/m2 -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_STDEXEC=ON -DCONCURRENCY_STUDY_STDEXEC_SOURCE_DIR="$PWD/build/full-windows/_deps/stdexec-src"
cmake --build build/m2 --config Release
ctest --test-dir build/m2 -C Release --output-on-failure
```

未启用依赖时，`#if CS_HAS_STDEXEC` 排除依赖头和实现，程序返回 77，明确说明缺少固定依赖。这个分支不是标准库后备实现，也不报告管线检查通过。主线程统一提供符合要求的 MSVC 预处理器选项；不要修改下载目录来临时规避版本差异。

## 自测与答案

**when_all 是否等于两个新线程？** 不等于。它组合子操作，执行资源决定怎样运行；同步 sender 甚至可以在当前线程完成。

**stopped 是否一定抛异常？** 本例 sync_wait 对 stopped 返回空 optional，error 才走重抛。调用者可以用 upon_stopped 显式转换策略。

**error 是否意味着兄弟任务已被强行杀死？** 不是。停止是协作的，汇合仍等子完成；不响应停止的业务代码必须有自己的有限结束条件。

**换一个 scheduler 是否就能把任意 lambda 放到 GPU？** 不能。代码、内存可访问性、错误模型和 scheduler 支持的操作都需要适配。可组合的抽象不消除执行资源的物理约束。

## 规范与实现入口

- [N5050 固定草案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[exec.when.all]`、`[exec.sync.wait]`、`[exec.then]`。通过条款名查找，避免页码因阅读器显示方式不同产生歧义。
- [NVIDIA stdexec 固定 commit](https://github.com/NVIDIA/stdexec/tree/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43)：对应课程实际编译的源码；下载目录中的 `include/stdexec/__detail/__sync_wait.hpp`、`__when_all.hpp` 可追踪实现。
- [P2300R10 历史设计](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html)：用于了解动机和演进，不替代 N5050 的固定基准。
