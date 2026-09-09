# 练习 9：environment 不是普通参数

## 目标

用 `get_scheduler()` 与 `get_stop_token()` 直接练 environment 查询，建立"执行上下文是从 receiver 一侧传下来的"这件事的直觉。

## 前置理解

- 你已经做完模块 B，知道 scheduler 可以显式进入图。
- 你知道 `get_scheduler()`、`get_stop_token()` 不是普通函数调用结果，而是 query sender。
- 你接受本题的重点是"上下文怎么被查询"，不是"取消机制细节全覆盖"。

## 必做任务

1. 创建一个线程池，并准备一个最小 payload，例如 `task_id`、计数值或请求编号。
2. 让一段 sender 在该线程池上开始执行。
3. 在这段图中显式读取当前 environment，至少读取两项：
   - `get_scheduler()`
   - `get_stop_token()`
4. 把 payload、当前 scheduler 查询结果、stop token 查询结果汇总成一个你自己定义的结构体，例如 `RuntimeContextSnapshot`。
5. 再利用查询到的 scheduler，继续构造一小段新 sender，让它在"当前 scheduler"上追加一段日志或处理。
6. 最终同步等待并输出快照信息。

## 进阶任务

- 把读取 environment 的逻辑封装成一个独立函数，返回 sender，再在主流程中组合它。
- 如果你固定的 `stdexec` 版本支持更明显的 stoppable 场景，额外实验 stop token 的可观察状态。
- 再做一个反例版本：把 scheduler 和 token 都当普通参数层层手动传，比较接口噪音。

## 验收点

- 你能运行出包含 scheduler 与 stop token 查询痕迹的结果。
- 你能解释这些值不是业务 payload，而是执行上下文。
- 你能说明为什么 environment 查询比手动逐层传参数更适合异步框架。
- 你能指出查询结果是在 sender 图的哪个阶段进入后续逻辑的。

## 常见坑

- 把 scheduler/token 直接写成外部捕获变量，结果没练到 query 模型。
- 把 query sender 当成立即求值的普通函数，导致心智模型混乱。
- 让快照结构体承担太多业务字段，反而看不清 environment 本身。
- 把停止语义理解成"肯定等于异常"。

## 对应官方参考

- `stdexec/examples/hello_world.cpp`
- `NVIDIA/stdexec` README 中 `get_scheduler` / `get_stop_token` 的示意
