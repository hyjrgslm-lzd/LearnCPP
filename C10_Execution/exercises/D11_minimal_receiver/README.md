# 练习 11：最小 receiver

## 目标

亲手写一个能接住三条 completion channel 的最小 receiver，把"receiver 不是抽象概念，而是真正参与执行的对象"刻进直觉。

## 前置理解

- 你已经知道 receiver 要面对 `set_value`、`set_error`、`set_stopped`。
- 你知道 receiver 还需要提供 environment 入口。
- 你能接受这题先从很小的 receiver 壳开始，不追求复杂模板技巧。
- 你已经看过上面的类型骨架桥接段。

## 必做任务

1. 定义一个 logging receiver，给它一个明显的名字，例如 `logging_receiver`。
2. 为它实现三类 completion 入口：
   - `set_value(...)`
   - `set_error(...)`
   - `set_stopped()`
3. 给它实现最小 `get_env()`，先返回一个足够简单的 environment 即可。
4. 准备三条最小 sender：
   - 一条正常 value 路径，例如 `just(42)`
   - 一条错误路径，例如 `just_error(...)`
   - 一条停止路径，例如 `just_stopped()`
5. 对每条 sender 都手动执行一次 `connect` 与 `start`，让你的 receiver 真正接住结果。
6. 记录三种 completion 分别打到 receiver 的哪一个入口。

## 进阶任务

- 让 receiver 把收到的事件记录到一个小型日志结构里，而不只是打印文本。
- 在 `set_error` 中区分异常类型或异常来源。
- 尝试给 `get_env()` 返回一个更接近官方示例的最小可查询环境，例如包含 `never_stop_token` 的属性。

## 验收点

- 你能手动完成 `connect` 与 `start`，而不是全程依赖 `sync_wait`。
- 你能观察到三条 completion channel 分别落在哪个成员函数。
- 你能说明 receiver 为什么必须出现在执行模型里，而不是只是"库内部回调对象"。
- 你能说出 `get_env()` 与 environment 查询之间的联系。

## 常见坑

- 以为 receiver 只需要 `set_value`，忽略错误和停止。
- `set_error` 只打印一行文本，完全看不出异常来源。
- `get_env()` 返回内容过重，反而把注意力从 receiver 本身带偏。
- 写出一个 receiver 类型，但没有真的手动 `connect` / `start` 它。
- 在编译错误面前放弃——回到桥接段检查骨架是否匹配。

## 对应官方参考

- `stdexec/examples/scope.cpp` 中的最小 receiver 风格
- `stdexec` 中 `just_error`、`just_stopped` 的基础用法
