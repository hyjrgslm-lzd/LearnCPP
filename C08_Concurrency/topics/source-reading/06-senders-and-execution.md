# 源码导读 06：sender 描述怎样变成活着的 operation state

课程已经能构造 schedule、then、when_all 和 sync_wait 管线。源码阅读还要回答：谁保存 receiver？什么时候才把工作放进队列？如果一条支路失败，其他支路尚未退出，谁防止 operation state 提前销毁？这几件事比背算法名更接近执行模型的核心。

本篇为可复验选择 NVIDIA/stdexec `nvhpc-26.05`，commit [`6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`](https://github.com/NVIDIA/stdexec/tree/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43)，与课程固定源码版本一致，不声称上游最新。2026-09-08 只读核查，不在本篇编译、运行第三方库。当前标准状态与标准/扩展区分留给[标准索引](../../references/standards-and-implementations.md)；这里的 `exec::static_thread_pool` 是这份实现提供的执行资源，不能当作同名标准设施。

先读 [任务组合桥接](../scheduling/03-execution-bridge.md)，真实课程使用点在 [M2 Reference](../../exercises/M2_execution_bridge/solution.cpp)。该练习已有的运行记录属于它自己的具体版本与配置，本篇只增加源码链的解释。完整自定义 sender/domain 教程仍由相邻 C10_Execution 承担，不在这里手写生产线程池。

## 1. 从聚合头继续往下走

`include/stdexec/execution.hpp` 是聚合入口，不能只链接它就算导读完成。选中 `schedule(pool.get_scheduler()) | then(f)`，分别进入：

| 问题 | 固定源码入口 |
|---|---|
| schedule 与实际资源 | [exec/static_thread_pool.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp) |
| then 的 sender 表达式与完成转换 | [__then.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__then.hpp) |
| 组合如何保存子 operation state | [__basic_sender.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__basic_sender.hpp) |
| 用户函数结果/异常怎样发给 receiver | [__receivers.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__receivers.hpp#L211) |
| 多支路完成与取消收束 | [__when_all.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__when_all.hpp) |
| 阻塞等待和结果出口 | [__sync_wait.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__sync_wait.hpp) |

这些内部双下划线名称是此 commit 的实现细节，不是建议应用直接调用的 API。阅读时跟踪它们，写业务代码时仍用公开接口。

## 2. connect 保存关系，start 才触发所选调度路径

线程池 scheduler 的 `_sender` 保存 pool 引用、remote queue 指针及调度约束。其 connect 返回 `_opstate<Receiver>`，将 receiver 与这些资源关系保存下来；`_opstate` 的 start 调 enqueue_(this)，这里才把代表该操作的 task 指针交给 pool。[schedule sender 与 connect](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp#L350)

这份 operation state 自身带任务节点形状。队列保存的是指向它的路径，不代表队列已经替调用者取得一个可以任意延长的 shared_ptr。调用方必须让 op 保持存活，直到相应完成协议允许销毁；connect 后还没 start，和 start 后尚未完成，是不同寿命阶段。[opstate 保存与 start](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp#L1201)

then_t 接受上游 sender 和函数，构造 sender 表达式，并不执行 f。通用 `__opstate` 在 connect 时构造 `__state_` 与 `__child_ops_`；子 receiver 引用这个 state，start 再把启动传递给子操作。该对象被明确设为不可移动，避免内部引用因为移动整个状态对象而失效。[通用组合 operation state](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__basic_sender.hpp#L281)

“惰性”在这里指这条 sender 路径尚未 start，不代表构造 sender/connect 永远不分配、不复制或不会抛异常；它也不代表所有 sender 都必须开新线程。本文的执行资源来自 schedule 的具体 pool，then 的普通成功函数在收到上游完成的上下文里执行，除非其他 adaptor/domain 改变了路径。

## 3. worker 如何发出 value 或 stopped

pool 的 worker `run` 从 thread_state.pop 得到 task，调用 task->execute_。schedule 的 `_opstate` 构造时安装这份执行函数：取得 receiver 环境里的 stop token，不可停止 token 直接 set_value；可停止 token 已有请求则 set_stopped，否则 set_value。请求被观察的检查点是这条 execute 路径，不能把 `request_stop()` 理解成异步强杀任何正在运行的用户函数。[run 与执行回调](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp#L804)

向下游发完成时，receiver 会被按约定移交；完成调用可以使拥有者准备销毁整个操作状态，所以实现不能在发完完成之后继续依赖已经失效的 op。这里是需要逐行检查的对象生命周期边界，不是“一个回调返回了所以任意 this 都还活着”。

线程池对象本身还有构造和退出路径：创建线程中途异常时 request_stop、join 后重抛；析构 request_stop 并 join。scheduler/operation state 保存的 pool 引用不能比 pool 活得更久，且不应在池销毁过程中继续新提交。池停机状态与某条 operation 的 stop token 又是两套责任，不能仅凭两个方法都叫 stop 就合并理解。[资源创建与销毁](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp#L770)

## 4. then 的三种完成出口

进入 `__then_impl::__complete_fn`，首先区分上游完成标签。如果是 set_value，调用 `__set_value_from` 执行保存的函数，再把函数结果发为下游 value；函数返回 void 时不制造一个伪造占位值。如果上游送来 error 或 stopped，then 直接转发该标签与参数，不执行成功变换 f。[then 完成转换](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__then.hpp#L62)

`__set_value_from` 的 try/catch 把允许抛异常的 f 的异常转换为 set_error(current_exception())。这个机制位于 adaptor 内部，和裸 thread 入口异常直接 terminate 不同。receiver 的完成操作本身需要满足其不抛出协议；不能让 receiver 任意抛出，再声称上层会像 future.get 那样替它兜底。

completion signatures 是这条转换的类型层说明：上游能送哪些 value/error/stopped，f 接受什么、返回什么、是否可能增加 exception_ptr 错误。先看 `__completions_t` 的变换，再对照运行时完成函数，才知道“签名允许的分支”和“这一输入实际经过的分支”是否一致。签名里出现 stopped 不等于每次都会取消；没有 value 也不等于应返回数字零。

## 5. when_all：有一个错误，不等于可以立刻销毁其他支路

when_all 的 `__state` 保存最终 receiver、原子计数 `__count_`、状态 `__state_`、stop_source、错误存储和各支路值存储。初始计数是子操作数；每个子 receiver 完成自己的值/错误处理后调用 `__arrive`，原子递减，最后一个到达者才 `__complete`。[状态与到达](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__when_all.hpp#L288)

收到第一个 error 时，`__set_error` 将状态切到 error，必要时向兄弟支路请求停止，并保存一个错误。之后的错误不覆盖已经选定的那个；多个并发错误的选择由实际竞争决定，不能按支路编号预测。构造值/错误存储本身若抛出，也需要进入相应 exception_ptr 路径。[错误与各支路完成](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__when_all.hpp#L484)

收到 stopped 时，仅在尚无 error 的状态下切到 stopped 并请求停止。error 优先于取消的处理在这个提交的状态机中明确存在；但最终是否可发送 stopped 还取决于相应类型路径，源码有 `_SendsStopped` 的编译期分支，不能删掉条件后宣称所有组合完全相同。

关键是：请求停止以后还要等所有子操作完成并到达。否则兄弟支路仍可能访问被 when_all 拥有的子 receiver、值槽或 stop_source，提前完成外层并销毁它就会留下 UAF。取消只是促使它们结束的请求，不是“把未完成子任务从计数里忽略”。

外部 stop callback 还有一个容易漏掉的保护：先临时增加 __count_，再转发请求，最后 arrive 撤回这份临时占用。停止回调可能同步、递归或与子完成并发发生，这个额外计数防止状态在转发函数仍使用时被最后一个子到达者结束。阅读时不能把 __count_ 永久解释成“纯粹还有几个业务任务”。[外部停止转发](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__when_all.hpp#L263)

## 6. sync_wait 的对象顺序与返回值

沿 `sync_wait` 的默认 apply_sender 路径，会看到局部 state 保存 exception_ptr 和 run_loop，另有 optional<tuple<...>> 结果；connect 形成局部 op，start(op)，再运行 loop。完成 receiver 的 set_value 将参数放入结果 optional，set_error 保存或转换异常，set_stopped 不填值；三条路径最后都调用 loop.finish。[等待入口](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__sync_wait.hpp#L300)

loop 返回后，如果有 eptr 就重抛；否则返回 optional。成功时 optional 有值，stopped 时为空。set_value 中构造 tuple 失败也会保存异常后结束等待，不把“计算成功但结果保存失败”伪装成一个有值结果。[等待 receiver](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__sync_wait.hpp#L81)

局部 op 存在到等待收束之后，而它引用的 state/result 声明得更早、销毁得更晚。这和课程 D1/D2 的对象顺序检查是同一个能力：不是因为库里有模板就可以停止画寿命图。同步等待仍可能长期阻塞；把它放到某个资源受限 worker 上等待同资源里的后续工作，也需要分析资源依赖，sender 名字不会自动消除嵌套等待死锁。

## 7. 和课程线程池/结果通道的契约对照

| 问题 | 教学版本 | 本篇 stdexec 路径 |
|---|---|---|
| 保存工作 | packaged_task 保存调用与结果状态 | sender 描述＋connect 形成 operation state；start 才启动所选路径 |
| 执行资源 | jthread 或课程有界池 | scheduler 引用 static_thread_pool，state 借用资源寿命 |
| 错误交付 | future.get 重抛 | then/set_error 在完成链传播，sync_wait 最终转换为抛异常 |
| 取消 | stop 请求与循环/等待检查 | receiver 环境 token、schedule 检查点、when_all 转发与收束 |
| 多任务结果 | 显式逐项 get/join | 各支路存储、到达计数、最终恰一次外层完成 |
| 背压与关闭 | 教学池有明确容量及拒收契约 | 不能给此 pool 自动附加相同容量/拒收 API；需另追对应提交路径 |

本篇读取的是这份实现的普通路径。domain 转换、定制 sender、bulk、NUMA 约束等可能改变具体状态与调度；这些名称出现于同一头文件，不代表已经全部读完或验证过。

## 8. 可复验任务与完整答案

令 `$src` 指向已有的固定 stdexec 检出；先核对 SHA。没有检出时在上面的网页找相同符号，不需构建依赖：

```powershell
git -C $src rev-parse HEAD
rg -n 'class _sender|auto connect|enqueue_\(|void start|execute_|void _static_thread_pool::run' "$src/include/exec/static_thread_pool.hpp"
rg -n '__complete_fn|__set_value_from|__make_sexpr' "$src/include/stdexec/__detail/__then.hpp"
rg -n 'struct __opstate|__child_ops_|constexpr void start' "$src/include/stdexec/__detail/__basic_sender.hpp"
rg -n '__arrive|__set_error|__forward_stop_request|__count_|request_stop' "$src/include/stdexec/__detail/__when_all.hpp"
rg -n 'set_value|set_error|set_stopped|connect\(|start\(|__loop_\.run|rethrow_exception' "$src/include/stdexec/__detail/__sync_wait.hpp"
```

**任务 A：指出这条 schedule 的“真正入队”发生在哪，不许只答 start。** 答案：scheduler::_sender::connect 创建 _opstate，_opstate::start 调 enqueue_(this)，再进入 pool.enqueue 的目标路径。connect 保存对象关系，入队借用 operation state 的任务节点，不能在 start 返回后立即销毁它。

**任务 B：上游 error/stopped 是否执行 then 的 f？f 抛异常又去哪？** 答案：非 value 直接转发，不调用成功函数；value 路径通过 __set_value_from 调 f，允许的函数异常转换成 set_error(exception_ptr)。不要把 receiver 违反 noexcept 协议的异常也算成受支持业务分支。

**任务 C：when_all 的一个支路失败，另一个忽略停止继续工作，外层能否先返回？** 答案：不能仅因已经选择错误结果就销毁状态；仍要等待另一支路 arrive。否则它持有的 state 引用会悬垂。错误决定最终结果类别，计数决定何时可以完成，两者职责不同。

**任务 D：为什么外部 stop callback 临时增加计数？** 答案：转发停止可能同步触发子完成，也可能与最后一个到达并发。临时占用保住状态，回调完成转发后再 arrive；不能把这项计数当作一个真实的新业务任务，也不能删掉当成冗余加减。

**任务 E：sync_wait 的三种出口及对象销毁顺序是什么？** 答案：value 对应有值 optional，error 经 eptr 重抛，stopped 对应空 optional；op 在等待收束前存活，state/result 比 op 更晚销毁。结果 tuple 构造失败也转为 error，不能用空 optional 混同所有失败。

**任务 F：这份源头能否证明“标准库已有 static_thread_pool”？** 答案：不能。它证明此固定 stdexec 快照提供该扩展执行资源，且课程桥接使用它；当前标准设施的名称与状态由标准索引核对。源码版本的新旧与是否属于标准是两个维度。

完成本篇不要求编写自定义生产 sender，只要求把公开表达式追到具体存储、启动、完成、失败与资源退出。将 commit、分支条件和这些出口附到答案，才是可独立复验的源码阅读证据。
