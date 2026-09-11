# D11 最小 receiver

receiver 是 completion 的落点。本题写一个记录型 receiver，亲手接住 value、error、stopped 三条 channel，并提供最小 environment。

## 知识

receiver 至少要能处理三类终结信号。只写 `set_value` 的 receiver 在真实组合中会丢掉失败和停止。`get_env` 是 receiver 向上游暴露上下文的入口。

## 机制

检查器连接三种 sender：`just(42, "records")` 触发 value，`just_error(exception_ptr)` 触发 error，`just_stopped()` 触发 stopped。Reference 把事件写入 `std::vector<event>`，而不是靠打印文本判断。

bad 版本把 stopped 当 value 记录，检查器拒绝。

## 必做

1. 定义 `event{channel, detail}`。
2. `set_value` 记录所有参数。
3. `set_error` 展开 `exception_ptr` 并记录消息。
4. `set_stopped` 记录 stopped。
5. `get_env` 返回带 receiver 名称的环境。

## 进阶

- 给 error 记录异常类型。
- 给 environment 加 stop token 或 trace id。

## 答案解释

正确 receiver 不拥有执行，它只接收 completion。每个 completion 入口都移动 receiver，符合“一次执行一条终结信号”的模型。
