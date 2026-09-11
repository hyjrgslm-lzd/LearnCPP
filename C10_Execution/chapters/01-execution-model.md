# 01 execution model: 工作描述、连接与启动

C++17 的并行算法和 sender/receiver 都在讨论“执行”，但它们解决的问题不同。`std::transform_reduce(std::execution::par, first, last, ...)` 把执行策略交给一个算法调用：算法实现可以分块、并行、向量化，也可以在当前标准库后端里顺序执行。调用表达式本身就是执行入口。

sender/receiver 先把工作描述成对象图。`just(record) | then(parse) | then(enrich)` 在构造时不解析记录，只保存“上游会发什么值、下游收到后做什么”。真正执行需要三步：构造 sender，`connect(sender, receiver)` 得到 operation state，`start(op)` 启动这一次运行。

## 五个核心对象

1. sender：描述可能完成的工作。它不是线程，也不是运行中的任务。
2. receiver：接收终结信号。它必须能处理 `set_value`、`set_error`、`set_stopped` 中适用的通道。
3. operation state：一次 sender 与 receiver 连接后的运行状态。它拥有本次运行需要的对象，通常不可移动。
4. scheduler：调度能力。`schedule(sch)` 产出 sender，表示“在这个执行资源上开始一段工作”。
5. environment：receiver 暴露给上游的查询入口。scheduler、stop token、allocator 等事实通过环境向上传递。

A2 验证惰性：构造 sender 后日志只有 `constructed` 和 `before sync_wait`，两个 `then` body 到 `sync_wait` 才运行。A3 再验证 `when_all` 也只是合流描述；构图时三条分支不能提前 fetch。

## 完成通道

value channel 携带正常结果。它可以是零个值、一个值、多个值，也可以因为某个 `then` 返回 `void` 变成空 value。A2 覆盖 `int`、结构体、`void` 和 move-only `unique_ptr`，防止把 value channel 写成共享外部状态。

error channel 携带失败。它不是一个特殊 value，也不能被伪造成默认值。G1 的 bad 变体把上游 error 改成 `set_value(-404)`，checker 明确拒绝。

stopped channel 表示未产出 value 的停止完成。停止请求本身不是完成；只有发出 `set_stopped` 才是终结信号。这个规则在后续 cancellation、scope、I/O 取消竞态里会继续出现。

## operation state 生命周期

`connect` 建立一次运行，可能分配、移动 sender 和 receiver，也可能因上游构造失败而抛异常。`start` 启动已经连接好的 operation state，必须 `noexcept`。已接受的 work 必须恰好发出一次终结信号；终结回调可能销毁 operation state，所以终结后不得再访问它。

A1、A2、A3 只展示最小 value 图。它们故意不承诺并行线程数，因为 sender 组合先表达工作关系；真正执行位置要等 scheduler 章节再进入。
