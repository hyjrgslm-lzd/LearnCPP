# Coroutine_Study 覆盖登记

核对日期：2026-09-08。本表登记 37 个学习单元的入口、练习形态、Reference 与当前状态。它只说明接线和检查含义；教学质量仍以正文审查、Reference 运行和学生实现检查共同判断。

## 覆盖口径

- 正文：课程主讲义或项目 README 中的机制说明。
- Part/Starter：学生独立编辑入口。
- Reference：完整答案或观察结果依据。
- 状态：`已接线` 表示入口存在并能纳入构建；`可选依赖` 表示需要对应 CMake 选项和本机依赖；`starter待完成` 表示学生代码仍有 TODO，不能作为通过证据。
- C01 工程先修桥接：[C01](../../Engineering_Study/README.md)提供编译链接、ABI和工具能力的连续讲解；[本课构建指南](../exercises/BUILD_GUIDE.md)保留协程专用的依赖、选项、CTest和超时操作。按当前单元需要补工程先修，不把整门C01作为入门协程的硬前置。
- 普通练习的程序目标采用下表练习目录名，Reference通常在目录名后加`_reference`；Capstone5包含多个`mini_reference_*`组件目标，具体注册见其[CMakeLists](../exercises/Capstone5_mini_corolib/CMakeLists.txt)。实际构建/执行集合和非作者审查统一从[质量报告](quality-report.md)回查，不把“已接线”当作全部平台验证通过。

## 37 单元

| 单元 | 正文入口 | Part/Starter | Reference/答案 | 目标 | 状态 |
| --- | --- | --- | --- | --- | --- |
| P1 | `00-预备知识-执行模型与标准库.md` | `exercises/P1_future_basics` | `solution.cpp` | future 共享状态、等待、异常 | 已接线 |
| P2 | `00-预备知识-执行模型与标准库.md` | `exercises/P2_generator_basics` | `solution.cpp` | generator 基础观察 | 已接线；用户本地 main.cpp 脏，本轮未改 |
| A1 | `02-模块A-三关键字与最小协程.md` | `exercises/A1_first_generator` | `solution.cpp` | 最小 `co_yield` 生成器 | 已接线 |
| A2 | `02-模块A-三关键字与最小协程.md` | `exercises/A2_co_return_lazy_task` | `solution.cpp` | lazy task 与 `co_return` | 已接线 |
| A3 | `02-模块A-三关键字与最小协程.md` | `exercises/A3_co_await_future` | `solution.cpp` | future awaiter 与跨线程恢复 | 已接线 |
| B1 | `03-模块B-generator与task的使用.md` | `exercises/B1_recursive_generator` | `solution.cpp` | 递归生成与嵌套产出 | 已接线 |
| B2 | `03-模块B-generator与task的使用.md` | `exercises/B2_task_sequential` | `solution.cpp` | 父子 task 顺序与异常 | 已接线 |
| B3 | `03-模块B-generator与task的使用.md` | `exercises/B3_callback_to_awaiter` | `solution.cpp` | 回调转 awaiter 的状态生命周期 | 已接线 |
| C1 | `04-模块C-取消与组合.md` | `exercises/C1_stop_token_cancel` | `solution.cpp` | stop token 协作取消 | 已接线 |
| C2 | `04-模块C-取消与组合.md` | `exercises/C2_when_all_when_any` | `solution.cpp` | 组合器结果、错误、取消 | 已接线 |
| C3 | `04-模块C-取消与组合.md` | `exercises/C3_async_scope` | `solution.cpp` | async scope 收束边界 | 已接线 |
| Capstone1 | `05-第一阶段结课-异步小爬虫.md` | `exercises/Capstone1_async_crawler` | `solution.cpp` | 抓取、解析、并发、取消 | 已接线 |
| D1 | `06-模块D-promise_type全解.md` | `exercises/D1_promise_8_hooks` | `solution.cpp` | promise 8 个关键定制点 | 已接线 |
| D2 | `06-模块D-promise_type全解.md` | `exercises/D2_eager_vs_lazy` | `solution.cpp` | eager/lazy 启动差异 | 已接线 |
| D3 | `06-模块D-promise_type全解.md` | `exercises/D3_final_suspend_symmetric` | `solution.cpp` | final suspend 与 symmetric transfer | 已接线 |
| E1 | `07-模块E-awaitable三层与co_await变换.md` | `exercises/E1_co_await_lookup` | `solution.cpp` | `operator co_await` 查找 | 已接线 |
| E2 | `07-模块E-awaitable三层与co_await变换.md` | `exercises/E2_await_suspend_three` | `solution.cpp` | `await_suspend` 三种返回 | 已接线 |
| E3 | `07-模块E-awaitable三层与co_await变换.md` | `exercises/E3_trivial_awaitable` | `solution.cpp` | ready 短路与 trivial awaiter | 已接线 |
| F1 | `08-模块F-协程帧与allocator.md` | `exercises/F1_frame_layout` | `solution.cpp` | frame 保存状态与生命周期 | 已接线 |
| F2 | `08-模块F-协程帧与allocator.md` | `exercises/F2_promise_allocator` | `solution.cpp` | promise 分配释放配对 | 已接线 |
| F3 | `08-模块F-协程帧与allocator.md` | `exercises/F3_halo_diagnose` | `solution.cpp` | HALO 观察与证据边界 | 已接线 |
| G1 | `09-模块G-symmetric_transfer与高级task.md` | `exercises/G1_shared_task` | `solution.cpp` | shared task 多等待者 | 已接线 |
| G2 | `09-模块G-symmetric_transfer与高级task.md` | `exercises/G2_when_all_impl` | `solution.cpp` | when_all barrier | 已接线；CTest timeout 5 秒保留 |
| G3 | `09-模块G-symmetric_transfer与高级task.md` | `exercises/G3_sync_wait_impl` | `solution.cpp` | sync_wait 单次启动与完成通知 | 已接线 |
| H1 | `10-模块H-协程与sender_receiver桥接.md` | `exercises/H1_as_awaitable` | `solution.cpp` | sender 到 awaiter 桥接 | 可选依赖：stdexec |
| H2 | `10-模块H-协程与sender_receiver桥接.md` | `exercises/H2_std_execution_task` | `solution.cpp` | C++26/stdexec task 探测 | 可选依赖：stdexec；Reference 按工具链条件构建 |
| H3 | `10-模块H-协程与sender_receiver桥接.md` | `exercises/H3_bidirectional_bridge` | `solution.cpp` | coroutine/sender 双向桥接 | 可选依赖：stdexec |
| I1 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I1_asio_echo` | `solution.cpp` | Asio echo 与事件循环 | 可选依赖：Asio |
| I2 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I2_io_uring_iocp` | `solution.cpp` | 平台 I/O 完成模型 | 可选依赖：IOCP 或 io_uring |
| I3 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I3_folly_safe_task` | `solution.cpp` | Folly task 生命周期 | 可选依赖：Folly |
| I4 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I4_cobalt_channel` | `solution.cpp` | Cobalt channel 生产消费 | 可选依赖：Boost.Cobalt |
| I5 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I5_cppcoro_patterns` | `solution.cpp` | cppcoro 模式对照 | 可选依赖：cppcoro |
| J1 | `12-模块J-陷阱诊断与跨编译器.md` | `exercises/J1_eight_pitfalls` | `solution.cpp` | 生命周期陷阱诊断 | 已接线；危险 demo 默认关闭 |
| J2 | `12-模块J-陷阱诊断与跨编译器.md` | `exercises/J2_cross_compiler_abi` | `solution.cpp` | 跨编译器 ABI 观察 | 已接线 |
| J3 | `12-模块J-陷阱诊断与跨编译器.md` | `exercises/J3_coroutine_tracing` | `solution.cpp` | 协程 trace 与执行链 | 已接线 |
| Capstone4 | `13-第三阶段结课-RPC框架.md` | `exercises/Capstone4_rpc_framework/src` | `reference/tests/rpc_reference_test.cpp` | Asio RPC、错误、超时、drain | 可选依赖：Asio；starter 运行返回 2 |
| Capstone5 | `14-第三阶段结课-mini协程库实现.md` | `exercises/Capstone5_mini_corolib/include/mini` 和 `tests` | `reference/tests` | mini 协程库核心与 stdexec 桥接 | 核心 Reference 已接线；starter 待完成 |

## 证据分类

- 规范：`references/标准条款与版本状态.md` 和各正文中的标准语义说明。
- 实现：`exercises/*/main.cpp`、`solution.cpp`、Capstone `include/src/reference`。
- 实验：CTest、单独可执行、可选依赖构建和正文中注明的诊断命令。

SKIP 只表示环境或依赖未满足，不能替代实现通过；starter 失败只表示 TODO 未完成，不能替代 Reference 验收。
