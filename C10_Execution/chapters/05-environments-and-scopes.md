# 05 environment 与 scope

environment 是 receiver 侧向上游暴露的执行上下文，不是业务 payload。payload 是这次工作处理的数据；environment 是“这次工作在哪里执行、能否停止、有哪些查询属性”。scope 解决另一类问题：operation state 被启动后由谁拥有、何时等待、错误如何收束。

## environment 查询

一个 sender adaptor 包装 receiver 时，最容易漏掉：

```cpp
auto get_env() const noexcept {
  return stdexec::get_env(downstream);
}
```

漏掉这行，上游看到的是包装 receiver 的空环境，而不是下游真正提供的 scheduler、stop token 或自定义属性。D13 的 `tap` 因此检查 environment 转发：上游 sender 在 `start` 里读 `get_env(receiver).marker`，如果 `tap_receiver` 没有转发，结果就不是 42。

C2_9 把 payload 和 environment 故意分开。payload 是业务字符串；receiver environment 通过 `write_env` 提供 `get_task_id`、`get_stop_token` 和 `get_scheduler`。正确实现必须用 `read_env` 读取这些 query，并用查询到的 scheduler 执行 follow-up work。bad 版本把 follow-up 留在 caller thread，检查器拒绝。

## scope 生命周期

sender 是描述，operation state 是一次执行实例。启动后，operation state 必须有所有者。`start_detached` 把所有权交给库内部的 detached 状态；调用者没有 rendezvous 点，无法知道工作是否都结束。结构化并发要求有 scope 作为拥有者：接受工作、拒绝关闭后的新工作、等待已接受工作 drain，再释放状态。

stdexec 的 `exec::async_scope` 是参考实现扩展，不是 N5050 标准接口。标准方向中的 task、counting scope、spawn 会在 `10-task-and-scope.md` 主讲；本章只讲扩展接口的工程语义，并把链接留给后续标准 scope 章节。两者共同原则相同：父作用域收束之前，子工作状态不能被释放。

C2_10 的可运行检查使用真实 `exec::async_scope`：`spawn` 接受 3 个 fire-and-forget 工作，`on_empty()` 等待它们 drain；`spawn_future` 返回结果 sender，调用方用 `sync_wait` 收束值和错误。检查器仍只看行为事实，避免用 sleep 猜调度。

## Part 对应

C2_9：

- Part 1：定义 snapshot，把 payload 与 environment 查询结果分开。
- Part 2：定义自定义 forwarding query `get_task_id`。
- Part 3：用 `read_env(get_task_id)`、`read_env(get_stop_token)` 和 `read_env(get_scheduler)` 读取真实 receiver environment。
- Part 4：用查询到的 scheduler 执行 follow-up，而不是手动传参或拼展示名。
- Part 5：解释为什么 query forwarding 是 adaptor 的责任。

C2_10：

- Part 1：区分 sender 描述、operation state 实例、scope 所有权、执行资源。
- Part 2：记录 accepted work，等待 drain 后再释放父作用域。
- Part 3：保存 future 风格结果和 error path，不能只数成功 value。
- Part 4：说明 `exec::async_scope` 是 stdexec 扩展；标准 task/counting scope 后续单独讲。

## 答案解释

Reference 代码故意保持小：C2_9 返回一条标准 sender 组合，用 `read_env` 从 receiver environment 取上下文；C2_10 用 `exec::async_scope` 验证真实生命周期不变量。good 独立接线，bad 能编译但把 scheduler follow-up 留在错误线程或提前释放，检查器按行为拒绝。

environment 与 scope 的共同点是：它们都不是“多传几个参数”能替代的。environment 是异步图的上下文查询面；scope 是异步实例的生命周期边界。
