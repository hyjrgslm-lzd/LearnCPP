# 09 从完成协议到运行时与环境组合

前几章的 sender 只是描述和组合工作。要把它落在另一条线程上，必须有人保存已接受工作，并在那个执行资源上调用完成函数。scheduler 是提交入口的可复制能力；运行时拥有队列、关闭状态和实际驱动循环。H1 从这条边界构造 run_loop，H2 再为同一条执行链传递请求级信息。

## 1. 队列里为什么放 operation state

`connect(schedule(loop), receiver)` 建立一次具体操作。sender 本身可以很短命；receiver、入队节点和 loop 引用必须留在连接结果中，直到完成。队列保存 operation state 地址，因此入队后移动它会使链表指针悬空。运行时不应把 sender 对象当作完成后的状态，也不应靠 detached 线程替代队列寿命设计。

本课 Reference 的 `c10_h1::run_loop` 使用侵入式 FIFO：操作节点自带 next 指针，loop 保存 head/tail。这样入队不用为每次操作另分配链表节点。代价是操作对象必须稳定且由调用者持有；不能先销毁连接结果再等待 loop 清空。Good 使用独立队列实现，验证行为契约并不要求内部布局相同。

```text
caller owns op(receiver, loop*) ── start ──> loop head/tail queue
                                             │ run() thread
                                             ▼
                                  remove node, release queue lock
                                             │
                                             ▼
                                  set_value(receiver) exactly once
```

`run()` 不自动创建线程。调用者可以在当前线程驱动，也可以创建 jthread 执行它。H1 检查完成回调的真实线程 ID，而不是查询 scheduler 的展示名称。H2 中查询到 scheduler 仅说明能力可见，调用 schedule 才把执行位置约束变成一次具体工作。

## 2. 从不变量推导锁和唤醒

队列的共享状态是 head、tail、closed。入队和出队都必须在同一把 mutex 下维护它们：空队列必须同时满足 head/tail 为空；非空时 tail 的 next 为空。条件变量等待谓词是 `closed || head != nullptr`，防止虚假唤醒和通知早于等待带来的丢失唤醒。

工作线程拿到节点后先在锁内断开链表，再释放锁执行用户 receiver。持锁执行用户代码会让 receiver 中的二次提交死锁，也会把一个慢任务变成对所有提交者的阻塞。完成函数允许直接销毁操作状态，因此调用 `op->execute()` 后不能继续读取 op 的成员；下一轮只访问 loop 自己的状态。

这不是无锁算法。mutex 明确建立入队数据发布到出队消费者的 happens-before；C08 主讲它的内存模型依据。本课关注这条同步关系如何支持 connect/start/completion 的对象契约。先把可证明的队列闭合，再用 B01 测量是否值得改变实现。

## 3. 关闭分成拒收与排空

只设置“退出”标记并立即返回会遗弃已接受工作：等待 receiver 永远收不到完成，拥有者也无法知道何时可以释放节点。H1 的 `close()` 把入口改为拒收；`run()` 仍排空已接受的工作，直到 closed 且队列为空才结束。

提交与 close 在锁下竞争：先取得锁并成功入队的操作被接受，后取得锁的操作通过 error channel 报告 `run_loop_closed`。这个线性化点让两种结果可解释。关闭不是 stopped，也不是强行杀掉已经接受的任务。需要合作式停止时，应另外传递 stop token；终结仍由各个操作负责。

检查顺序有两种互补形状。第一种先开启驱动线程，提交多个节点，验证 FIFO 和真实执行线程；第二种先提交，再 close，最后 run，证明关闭没有丢弃队列。随后再次提交，必须得到明确错误。线程 join 和队列 drain 都完成后才销毁 loop；析构函数不能凭一个布尔标志证明其他线程退出。

## 4. 为什么需要 query，而不是给每个 sender 加字段

记录流水线可能同时需要 scheduler、trace ID、停止 token 和内存分配策略。若每个 adaptor 都给这些属性增加构造参数，新增一个 query 就要修改整张图。Environment 把“可询问的上下文”独立出来，receiver 的 `get_env()` 提供环境对象，各 query 自己定义返回值和语义。

H2 的 `get_trace_id` 是空 CPO，通过 `env.query(get_trace_id_t{})` 成员定制；环境只有支持该属性才提供重载。与字符串字典相比，缺失查询是编译期可识别的，返回类型/引用类别/noexcept 也属于类型接口。E2 保留 tag_invoke 历史，本题对应当前固定 stdexec 的成员接口，不能把两代写法混成一个规范要求。

## 5. 通用覆盖与回退怎样工作

设组合器保存 Base 和 Override。对任意 Q，只有三种情况：Override 可查询则命中它；否则 Base 可查询则回退；两者都不支持则当前 query 重载不参与匹配。Reference 用两组 requires 表达这条规则；它从不列举 trace、配额或 scheduler 的具体类型。

```text
Q(composed)
  ├─ requires Q(override) 成立 → Q(override)
  └─ 否则 requires Q(base) 成立 → Q(base)
       └─ 仍不成立 → query 不可调用
```

优先顺序必须由可查询性决定，而不是“返回空字符串就试父层”。空 trace 或零配额可能是合法值，不能同时承担未定义标记。组合时按值保存两个环境，因此原父环境保持不变；嵌套组合可以局部覆盖配额，同时继承父 trace 与 scheduler。

返回 `decltype(auto)` 保留查询的借用语义，noexcept 也取自实际选中的查询分支。假设 trace query 返回字符串引用，组合器不能无意复制为临时值；反过来，离开环境寿命后继续持有该引用同样错误。H2 的 sender 图在 `then` 内转成拥有的 string，再交给 sync_wait 的结果。

## 6. forwarding query 是传播许可

环境组合器能回答 query，不保证 query 穿过所有 adaptor。`when_all` 等实现会创建自己的 receiver 环境，例如叠加共享 stop token，同时只转发应传播的属性。H2 自定义 CPO 明确响应 `query(stdexec::forwarding_query_t)` 为 true，让 trace/配额穿过这种过滤层。

这个声明是属性契约，不能对所有未知 query 全部转发以图省事。某些查询只在当前层有意义。实验中删除 trace 的 forwarding 声明，会在 `read_env` 的完成签名推导阶段报告环境没有对应属性；这比返回预填 trace 更能说明查询链实际经过了哪些层。

H2 最终由 `write_env(when_all(read_env(trace), read_env(quota)), nested_env)` 验证真实 receiver 路径，并另查 scheduler 对象相等性。Reference 手写组合，Good 使用固定库 `stdexec::env{override, base}`，Bad 反转优先级。四组输入加嵌套覆盖防止只针对一个展示值写答案。

## 7. 练习顺序与下一步

先完成[H1](../exercises/H1_run_loop/README.md)的连接对象、入队/出队、关闭排空和实际驱动线程；再完成[H2](../exercises/H2_custom_query/README.md)的 query、覆盖回退、SFINAE 与真实 sender 环境传播。每一步先画状态/所有权，再预测对应检查的完成位置和通道。

H3 会把同样的环境查询放进 coroutine promise，并把完成函数接到恢复点；I1 则把队列中的完成源换成操作系统通知。两者都继承这里的底线：接受之后必须终结，终结之后不能访问可能已销毁的状态，停止和关闭不能代替资源收束。
