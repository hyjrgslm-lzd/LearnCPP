# C09_Coroutines 覆盖登记

核对日期：2026-09-11。本表登记 37 个学习单元的入口、练习形态、Reference 与当前状态。入口接线不等于作业完成；最终运行、能力缺失和非作者结论分别见质量报告及本轮审计表。

## 覆盖口径

- 正文：课程主讲义或项目 README 中的机制说明。
- Part/Starter：学生独立编辑入口。
- Reference：完整答案或观察结果依据。
- 状态：`已接线` 表示入口存在并能纳入构建；`可选依赖` 表示需要对应 CMake 选项和本机依赖；`starter待完成` 表示学生代码仍有 TODO，不能作为通过证据。
- C01 工程先修桥接：[C01](../../C01_Build_Compile_Link/README.md)提供编译链接、ABI和工具能力的连续讲解；[本课构建指南](../exercises/BUILD_GUIDE.md)保留协程专用的依赖、选项、CTest和超时操作。按当前单元需要补工程先修，不把整门C01作为入门协程的硬前置。
- 普通练习的程序目标采用下表练习目录名，Reference通常在目录名后加`_reference`；Capstone5包含多个`mini_reference_*`组件目标，具体注册见其[CMakeLists](../exercises/Capstone5_mini_corolib/CMakeLists.txt)。实际构建/执行集合和非作者审查统一从[质量报告](quality-report.md)回查，不把“已接线”当作全部平台验证通过。

## 37 单元

| 单元 | 正文入口 | Part/Starter | Reference/答案 | 目标 | 状态 |
| --- | --- | --- | --- | --- | --- |
| P1 | `00-预备知识-执行模型与标准库.md` | `exercises/P1_future_basics` | `solution.cpp` | future 共享状态、等待、异常 | 已接线 |
| P2 | `00-预备知识-执行模型与标准库.md` | `exercises/P2_generator_basics` | `solution.cpp` | generator 基础观察 | 观察入口保留；历史脏状态不沿用为当前结论 |
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
| I2 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I2_io_uring_iocp` | `windows/solution_iocp.cpp`、`linux/solution_io_uring.cpp` | 平台 I/O 完成模型 | Windows IOCP 与 WSL io_uring 分别实测 |
| I3 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I3_folly_safe_task` | `solution.cpp` | Folly task 生命周期 | 可选依赖：Folly |
| I4 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I4_cobalt_channel` | `solution.cpp` | Cobalt channel 生产消费 | 可选依赖：Boost.Cobalt |
| I5 | `11-模块I-真实异步IO与并发框架.md` | `exercises/I5_cppcoro_patterns` | `solution.cpp` | cppcoro 模式对照 | 可选依赖：cppcoro |
| J1 | `12-模块J-陷阱诊断与跨编译器.md` | `exercises/J1_eight_pitfalls` | `solution.cpp` | 生命周期陷阱诊断 | 已接线；危险 demo 默认关闭 |
| J2 | `12-模块J-陷阱诊断与跨编译器.md` | `exercises/J2_cross_compiler_abi` | `solution.cpp` | 跨编译器 ABI 观察 | 已接线 |
| J3 | `12-模块J-陷阱诊断与跨编译器.md` | `exercises/J3_coroutine_tracing` | `solution.cpp` | 协程 trace 与执行链 | 已接线 |
| Capstone4 | `13-第三阶段结课-RPC框架.md` | `exercises/Capstone4_rpc_framework/src` | `reference/tests/rpc_reference_test.cpp`；public API 答版 `good/` | Asio RPC、错误、超时、drain | Asio；Student 实际调用安全 TODO 后失败，答版/独立 protocol good/Reference 分列 |
| Capstone5 | `14-第三阶段结课-mini协程库实现.md` | `exercises/Capstone5_mini_corolib/include/mini` 和 `tests` | `reference/tests` | mini 协程库核心与 stdexec 桥接 | 核心 Reference 已接线；starter 待完成 |

## 本轮逐单元审计与反向链

每行列出的单元均已逐项审计；同类仅合并呈现，未删除原题。具体 Part → 操作 → 检查 → 解析在各题 README；原始失败、修复和版本记录在对应作者报告，独立结论在 reviews 中。`无需改` 表示无需重写已有正文，仍要参加所适用的验证。

| 单元 | 处置 | 下游任务与必要先修 | 证据／审查入口 |
|---|---|---|---|
| P1、P2 | 无需改，保留观察 | A3 future 通知、B1 递归 generator；先区分共享状态与协程状态 | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| A1、A2 | 无需改，保留基础实现/观察 | B2、D 中使用 owner 与 yield/return；先理解 initial/final suspend | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| A3 | 学生检查补强 | 爬虫与 I/O 桥接；future 未就绪、完成位置、状态存活 | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| B1、B2 | 学生检查补强 | 递归产出与顺序流水线；引用寿命、elements_of、值/异常传递 | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| B3 | 检查与 PASS 边界补强 | H/I；回调结果确实经过 await_resume，同步窗口另列解析/扩展 | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| C1 | 检查与 PASS 边界补强 | 爬虫、RPC；协作取消与结果/异常，长计算不检查 token 的反例仍作观察 | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| C2、C3 | 行为检查与教学补强 | G2、scope；全部子项先登记再释放，协程并发不等于不同 OS 线程 | [R3 反例与修复](validation/c09-refresh/authors/foundation/r3-gate-same-thread-good-bad.json) |
| Capstone1 | 代码/检查/错误路径补强 | generator 解析、task 组合、停止与排空；移动后错误 URL 在原失败上修复 | [基础审计](validation/c09-refresh/authors/foundation/audit.md) |
| D1、D3 | 学生检查补强 | G/mini task；promise 完成与 continuation 转交，不把机器栈性质当标准保证 | [机制报告](validation/c09-refresh/authors/mechanisms/author-report.md) |
| D2、E1、E2 | 无需改，保留观察与解析 | E3/H3；启动策略、查找、await_suspend 三返回形态 | [机制报告](validation/c09-refresh/authors/mechanisms/author-report.md) |
| E3、F1、F2 | 证据边界审计 | F3/mini 库；ready、状态/分配观察不能单靠耗时推断 HALO | [机制报告](validation/c09-refresh/authors/mechanisms/author-report.md) |
| F3 | 实验与证据补全 | 分配消除判断；同契约基线、计数与无插桩 IR、独立进程样本分开 | [机制报告](validation/c09-refresh/authors/mechanisms/author-report.md) |
| G1 | 明确有限单线程基线，衔接可注销等待者实现 | mini shared_task；多消费者、结果缓存与等待者寿命 | [runtime 修复审查](validation/c09-refresh/reviews/runtime-review.md) |
| G2 | 学生 fan-out 检查补强 | mini when_all；登记/完成 barrier，不能用串行 drain 冒充组合 | [机制报告](validation/c09-refresh/authors/mechanisms/author-report.md) |
| G3 | 复杂样章与异常所有权补全 | 全部 task 消费；结果移动异常仍释放帧、final 通知期间状态保活 | [样章审查](validation/c09-refresh/reviews/s1-review.md) |
| H1、H2、H3 | Student 三链/探针/标准边界补强 | mini/应用；C10 sender 基础、操作状态、完成通道与 await 变换 | [桥接报告](validation/c09-refresh/authors/bridges/summary.md) |
| I1、I2 | 真实 I/O 与 Student 检查补强 | RPC；C07 完成模型、buffer/handle 存活和关闭收束 | [桥接报告](validation/c09-refresh/authors/bridges/summary.md) |
| I3、I4 | 完整入口保留，检查骨架补强，运行未验证 | 固定 Folly/Cobalt 的 task/channel、executor 与所有权；当前缺依赖不缩减正文 | [桥接报告](validation/c09-refresh/authors/bridges/summary.md) |
| I5 | 实际学生操作检查与版本边界补强 | cppcoro 对照阅读；TS 历史实现不冒充标准 API | [桥接报告](validation/c09-refresh/authors/bridges/summary.md) |
| J1、J2、J3 | 观察/诊断主体保留，实际复验 | 全课诊断；frame/闭包寿命、ABI 与 trace 的实现边界 | [教学/实验审查](validation/c09-refresh/reviews/content-experiment-review.md) |
| Capstone4 | 契约、学生接口、逐阶段检查补全 | C05 字节、C07 I/O、H/I 生命周期；协议、pending、取消、重试、drain | [RPC 审查](validation/c09-refresh/reviews/rpc-review.md) |
| Capstone5 | 生命周期修复、学生检查与独立可做性补全 | D–G；scope 启动事务、shared waiter 注销、producer 寿命、一次性恢复 | [runtime 审查](validation/c09-refresh/reviews/runtime-review.md)、[Student good/bad](validation/c09-refresh/reviews/mini-student-review.md) |

RPC 的完整 `good/` 是匹配 Student public API 的 Reference-adapted 答版，不计为 clean-room good；独立 protocol 实现只证明 protocol Part。观察型通过、Student 起点的预期拒绝、Reference 通过、能力缺失分别报告，不相加为“学生已完成 37 题”。

## 证据分类与历史路径

- 规范：`references/标准条款与版本状态.md` 和各正文中的标准语义说明。
- 实现：`exercises/*/main.cpp`、`solution.cpp`、Capstone `include/src/reference`。
- 实验：CTest、单独可执行、可选依赖构建和正文中注明的诊断命令。

SKIP 只表示环境或依赖未满足，不能替代实现通过；starter 失败只表示 TODO 未完成，不能替代 Reference 验收。

2026-09-09 之前的日志可能引用 `Coroutine_Study/`。历史正文、失败和指纹保持原件；重跑时映射为 `C09_Coroutines/` 并使用新的 build 目录。本轮来源与复现入口见 [实施规格](implementation-spec.md)和[质量报告](quality-report.md)。
