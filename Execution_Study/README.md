# `std::execution` 练习包

## 这套文档要解决什么问题

这不是一套"背 API"的笔记，而是一套"通过亲手编码理解框架设计"的练习包。

截至 2026-03-31，在 Visual Studio 2026 / MSVC 的公开资料里，标准库 `<execution>` 仍主要对应 C++17 并行算法执行策略，如 `seq`、`par`、`par_unseq`；如果你要练的是 C++26 / P2300 的 sender-receiver 模型，那么本地最现实的主线是用 `stdexec` 来学习 `std::execution` 的设计思想。

因此，这套文档采取两层定位：

- 热身层：理解现有 MSVC `<execution>` 与并行算法执行策略。
- 主线层：用 `stdexec` 练 sender、receiver、scheduler、operation_state、environment 这套模型。

## 你会得到什么

这套练习包分为两个阶段：

### 第一阶段：框架使用与概念理解

- 1 份心智模型总说明。
- 4 个模块（A/B/C1/C2/D），共约 15 道练习题。
- 1 个结课项目 + 1 条源码阅读路线。
- 每题统一的复盘框架。

### 第二阶段：实现技巧与架构设计

- 4 个模块（E/F/G/H），共 12 道练习题。
- 2 个结课项目（实现级源码对照 + mini std::execution 子集实现）。
- 覆盖 tag_invoke/CPO、类型级计算、sender adaptor 实现、run_loop、协程桥接实现等核心技术。

### 总计

- **~27 道练习题 + 4 个结课项目**

## 阅读顺序

### 第一阶段

1. `01-心智模型.md`
2. `02-模块A-惰性与组合.md`
3. `03-模块B-调度与执行上下文.md`
4. `04-模块C1-错误与取消.md`
5. `05-模块C2-环境与作用域.md`
6. `06-模块D-自定义sender与协程桥接.md`
7. `07-结课项目与源码阅读路线.md`（第一阶段结课）

### 第二阶段

8. `08-模块E-定制化机制.md`
9. `09-模块F-类型级技术.md`
10. `10-模块G-sender-adaptor实现.md`
11. `11-模块H-高级实现模式.md`
12. `12-结课项目2-实现级源码阅读.md`（第二阶段结课）

## 统一技术基线

本练习包默认你已经具备本地编码条件，因此这里不写安装和工程搭建，只固定练习边界。

- 语言基线：`C++20`
- 学习主线：`stdexec`
- 练习范围：CPU 场景
- 常用头文件：`<stdexec/execution.hpp>`、`<exec/static_thread_pool.hpp>`、`<exec/async_scope.hpp>`
- 排除范围：GPU、`nvexec`、`io_uring`、Linux-only 例子
- 线程要求：除非题目特别说明，不要手写 `std::thread`

## 两个阶段的定位差异

### 第一阶段：你学的是"怎么用"

- 理解 sender-receiver 的五个核心对象
- 掌握 just/then/when_all/let_value/upon_error 等组合器
- 理解惰性、显式调度、结构化并发、三条 completion channel
- 能手写最小 sender/receiver/operation_state
- 能阅读官方示例并理解对象关系

### 第二阶段：你学的是"怎么做"

- 理解 tag_invoke/CPO/niebloid 这套定制化机制及其演进
- 理解 completion_signatures 的类型级计算和传播
- 能实现 sender adaptor（inner receiver + channel 拦截模式）
- 能实现 pipe 语法、retry 组合器
- 能实现 run_loop、自定义 query、environment 组合器
- 能实现 coroutine promise_type 与 sender 的桥接
- 能阅读 stdexec 源码并指出实现模式

## 你要始终记住的定位

### 1. `std::execution` 有两个容易混淆的层面

- 你在 MSVC 文档里看到的 `<execution>`，当前主要是并行算法执行策略。
- 你真正想学的 P2300 `std::execution`，是 sender-receiver 异步执行框架。

这两者有关联，但不是一回事。前者更像"给算法一个执行策略标签"；后者更像"用类型系统描述一张异步工作图"。

### 2. 本地练习时，代码命名会分成两层

- 标准概念层：写作 `std::execution`
- 参考实现层：代码使用 `stdexec::` 与 `exec::`

### 3. 这套文档不追求"最短可运行代码"

它追求的是：

- 你能把异步工作拆成图
- 你能说清值、错误、停止三个 completion channel
- 你能说清调度器、operation_state、作用域对象分别负责什么
- 你能从官方示例里读出抽象对象，而不是只看语法糖
- （第二阶段）你能解释框架的实现选择，并能实现其中的关键子集

## 每题统一交付物

每完成一道题，至少留下四样东西：

1. 一份可运行代码。
2. 一张 sender graph 草图。
3. 一段 5 到 10 行的观察记录。
4. 一段复盘结论：这一题到底让你理解了什么设计点。

## 每题统一模板

所有练习题都按同一模板组织：

- 目标
- 前置理解
- 必做任务
- 进阶任务
- 验收点
- 观察点
- 常见坑
- 提示
- 复盘问题
- 对应官方参考

你做题时也尽量按这个模板留笔记。这样你后面回看时，会非常容易发现自己到底卡在"概念没懂"，还是"API 没用熟"，还是"实现模式没理解"。

## 建议节奏

### 方案 A：第一阶段 6-8 天

- 第 1 天：`01` + 模块 A
- 第 2 天：模块 B 前两题
- 第 3 天：模块 B 第三题 + 模块 C1 第一题
- 第 4 天：模块 C1 第二题 + 模块 C2 第一题
- 第 5 天：模块 C2 第二题 + 模块 D 前两题
- 第 6 天：模块 D 后两题
- 第 7-8 天：结课项目

### 方案 B：第二阶段 8-10 天

- 第 1-2 天：模块 E（定制化机制）
- 第 3-4 天：模块 F（类型级技术）
- 第 5-6 天：模块 G（sender adaptor 实现）
- 第 7-8 天：模块 H（高级实现模式）
- 第 9-10 天：结课项目

### 方案 C：慢练，全程 20 天

- 每天只做 1-2 题
- 每两题安排一次源码回看
- 结课项目各预留 2 天

## 统一判定标准

如果你做完一道题，只是"代码跑了"，那还不够。至少再检查下面四件事：

- 你能指出真正开始执行的时刻在哪里。
- 你能指出谁拥有这次执行的生命周期。
- 你能指出值是怎么流动的，错误和停止又会怎么流动。
- 你能指出这道题里 scheduler 到底有没有显式出现，如果出现了，它承担了什么角色。

第二阶段额外检查：

- 你能指出这道题涉及了哪种 C++ 实现技术（CPO/tag_invoke/类型计算/inner receiver 等）。
- 你能指出这种技术解决了什么问题，以及它的替代方案是什么。

## 术语速查

| 术语 | 你在练习里会看到什么 | 你应该问自己的问题 |
| --- | --- | --- |
| sender | `just(...)`、`schedule(sch)`、`when_all(...)` 的结果对象 | 它是在描述工作，还是已经开始工作？ |
| receiver | 自定义 logging receiver、`sync_wait` 内部消费端 | 结果最终交给谁？ |
| operation_state | `connect(sender, receiver)` 的结果 | 哪个对象真正代表"一次执行实例"？ |
| scheduler | `pool.get_scheduler()` | 它代表线程，还是代表一类可调度能力？ |
| environment | `get_scheduler()`、`get_stop_token()` 的查询来源 | 这些上下文是谁往下传的？ |
| completion_signatures | `completion_signatures<set_value_t(int), ...>` | 这个 sender 在编译期承诺了哪些完成方式？ |
| value / error / stopped | `then`、`upon_error`、`upon_stopped` | 这次完成究竟走了哪条通道？ |
| CPO | `connect`、`set_value` 等全局函数对象 | 这个"函数"为什么是对象而不是函数模板？ |
| tag_invoke | `friend tag_invoke(connect_t, ...)` | 为什么所有定制都走同一个 ADL 入口？ |
| inner receiver | sender adaptor 内部包装的 receiver | 它拦截了哪个 channel，其余如何转发？ |

## 统一编码约束

- 每题先写最小可观察版本，再做"进阶任务"。
- 优先记录线程 ID、阶段名、输入输出值形状。
- 不要过早追求通用库封装，先把对象关系画清楚。
- 不要把共享可变状态当作默认方案，优先让值沿 sender 图流动。
- 不要为了"像并行 STL"而把 sender 图写成一大团 lambda；sender 图的目标是显式组合，不是隐藏步骤。

## 一个非常重要的现实提醒

`stdexec` 是参考实现，而且是实验性质项目。你本地固定的 commit 与未来版本可能存在 API 细节差异。所以如果某个练习里的写法在你固定版本上略有出入，优先保持"设计意图一致"，不要把精力浪费在追求字面拼写完全相同上。

最典型的差异是：某些适配器既可能支持管道写法，也可能更适合函数式调用。比如 `let_value`，如果你本地版本的管道形式编译体验不好，就直接改成函数式调用。

同样，`tag_invoke` 与 member-function dispatch 在不同版本中的支持程度可能不同。第二阶段的练习以理解设计意图为主，不纠结于某个特定 commit 的 API 形式。

## 推荐做题方法

每题都按下面的顺序推进：

1. 先用一句话写出你认为这题在训练什么。
2. 先画 sender graph，再落代码。
3. 先做"必做任务"，不要一开始就追进阶。
4. 跑通后，不马上进入下一题，先回答复盘问题。
5. 每做完一个模块，回去重读一次 `01-心智模型.md`。

## 做完整套之后你应该达到什么水平

### 第一阶段完成后

- 解释为什么 sender 是"工作描述对象"而不是"工作线程"。
- 解释为什么 operation_state 是 sender/receiver 模型里必须单独存在的一层。
- 解释为什么 scheduler 必须显式成为图的一部分，而不是隐藏在库内部。
- 解释为什么 environment/query 能比手动层层传参更适合异步框架。
- 解释为什么 `stopped` 不能简单等同于 `error`。
- 看懂 `hello_world`、`scope`、`hello_coro` 这种示例背后的对象关系。

### 第二阶段完成后

- 解释 tag_invoke / CPO / niebloid 的设计意图和演进历史。
- 实现一个 sender adaptor（inner receiver + channel 拦截 + 签名变换）。
- 实现 completion_signatures 的编译期计算和变换。
- 实现 run_loop 和 intrusive data structure。
- 实现 coroutine promise_type 与 sender-receiver 的桥接。
- 阅读 stdexec 源码时能指出实现模式。
- 从零实现一个 mini std::execution 子集。

## 参考资料入口

做题过程中，建议反复对照下面这些资料的"概念定位"，而不是一上来通读全文：

- Microsoft Learn: `<execution>`
- Microsoft Learn: `/std` 编译开关说明
- `NVIDIA/stdexec` README
- `stdexec/examples/hello_world.cpp`
- `stdexec/examples/scope.cpp`
- `stdexec/examples/hello_coro.cpp`
- P2300R10 `std::execution`
- P3090R0 `std::execution Introduction`
- P3143R0 `An in-depth walk-through of the example in P3090R0`
- P1895R0 `tag_invoke: A general pattern for supporting customisable functions`
- P2855 (member-function-based customization direction)

## 最后一句提醒

不要把这套练习当成"我要赶快会写多少个算法"。

把它当成两层训练：第一层是框架使用训练——你在学习一种把异步工作表示、组合、启动、收束、传播上下文的方式。第二层是框架实现训练——你在学习用什么 C++ 技术来构建这种框架。

两层都练透，你对 C++ 异步编程的理解就不再停留在"能用"，而是到达"能设计"。
