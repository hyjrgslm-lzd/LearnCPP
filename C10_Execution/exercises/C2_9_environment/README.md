# C2_9 environment 不是 payload

payload 是业务数据；environment 是 receiver 侧提供的执行上下文。本题要求 sender 图通过 `stdexec::read_env` 读取上下文，而不是把 scheduler、stop token 或 task id 当普通参数层层传。

## 知识

sender/adaptor 连接 receiver 后，上游可以通过 environment 查询 scheduler、stop token、allocator 或自定义属性。包装 receiver 的 adaptor 必须转发 `get_env`，否则上游看到的上下文会丢失。

## 机制

checker 用 `write_env` 注入三类上下文：

- 自定义 query `get_task_id`；
- 标准 query `stdexec::get_stop_token`；
- 标准 query `stdexec::get_scheduler`。

正确实现返回的 sender 图必须用 `read_env(get_task_id)`、`read_env(get_stop_token)` 和 `read_env(get_scheduler)` 真正读取这些值，并用查询到的 scheduler 执行一次 follow-up work，记录 follow-up thread id。checker 覆盖 `stop_requested=false/true`；follow-up 子图用 `never_stop_token` 避免测试自身被取消，stop 状态仍必须来自外层 environment。payload 只用于 `snapshot.payload`，不能携带 scheduler 或 stop 状态。

## 必做

1. 定义 `runtime_context_snapshot` 和 `get_task_id` query。
2. `read_runtime_context(payload)` 返回 sender 图。
3. sender 图用 `read_env` 读取 task id、stop token 和 scheduler。
4. follow-up 必须通过查询到的 scheduler 执行。
5. snapshot 中区分 payload、task id、stop 状态、start thread 和 follow-up thread。

## 答案解释

Reference 不在 `start` 里手动拷贝 fake env 字段，而是返回标准 sender 组合：`when_all(read_env(...), read_env(get_scheduler) | let_value(schedule(...))) | then(...)`。bad 版本能编译，但 follow-up 留在 caller thread，检查器按行为拒绝。
