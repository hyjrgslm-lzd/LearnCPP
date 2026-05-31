# 练习 7：错误也是 completion channel

## 目标

在 sender 图里处理异常路径，建立"错误属于图的一部分，而不是图外补丁"的习惯。

## 前置理解

- 你已经理解 value channel 的流动方式。
- 你知道某个 `then` 阶段可以抛异常。
- 你知道 `upon_error` 的作用是把 error completion 转成新的值。
- 你知道 `let_error` 的作用是把 error completion 转成新的 sender。

## 必做任务

1. 设计一个很小的解析流程，例如把字符串解析成请求对象、记录对象或命令对象。
2. 约定一类非法输入，例如空字符串、字段数不对、数值格式错误。
3. 在某个 `then` 阶段里对非法输入直接抛异常。
4. 使用 `upon_error` 把这条 error path 转换成一个兜底结果，例如 `BadRequestResult`、默认对象或带错误码的响应结构。
5. 让最终阶段统一返回一个结果对象，而不是把异常直接抛回最外层。
6. 分别用一组合法输入和一组非法输入运行，记录两条路径的日志。
7. 再做一个版本：把 `upon_error` 替换为 `let_error`，在 error path 中返回一个新的 sender（例如 `just(fallback_result)` 或一段包含日志记录 + 值构造的 sender 链），而非直接返回值。对比两种写法的差异：`upon_error` 是"值级恢复"，`let_error` 是"sender 级恢复"——后者可以在恢复路径中继续做异步工作。

## 进阶任务

- 把兜底结果设计成和正常结果同一类型，但带不同状态字段，观察合流之后的接口形状。
- 再做一个版本：故意在 `upon_error` 之后的阶段里继续抛异常，确认 error path 不是"一次处理后世界清净"。
- 如果你本地固定版本支持 `upon_stopped` 的基础实验，再额外体会停止路径与错误路径的差别。
- 用 `let_error` 构造一个简单的重试逻辑：error 时返回一个"重新解析同一输入"的 sender，最多重试一次。体会 `let_error` 为什么是构建重试模式的基础。

## 验收点

- 合法输入走 value path，非法输入走 error path，最终都能得到统一结果对象。
- 你没有在图外层处处散落 `try/catch`。
- 你能明确指出 `upon_error` 和 `let_error` 在 sender 图中的位置及差异。
- 你能说明为什么 `let_error` 比 `upon_error` 更适合需要异步恢复的场景。

## 常见坑

- 把所有异常都拖到 `sync_wait` 外层统一捕获，结果练不到图内恢复。
- `upon_error` 里返回的对象类型与 value path 不协调，导致后面接口很别扭。
- 在多个阶段随意抛不同类型异常，最后自己也分不清哪条 error path 在起作用。
- 误以为一旦用了 `upon_error`，后续图就再也不会失败。
- 混淆 `upon_error` 和 `let_error`：前者返回值，后者返回 sender。

## 对应官方参考

- `stdexec/examples/server_theme/then_upon.cpp`
- `stdexec` 中 `upon_error`、`let_error`、`upon_stopped` 相关基础示例
