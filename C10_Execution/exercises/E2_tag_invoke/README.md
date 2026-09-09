# 练习 E-2：tag_invoke 基础设施

## 目标

实现 `tag_invoke` 基础设施，并用它来定制 `connect` 协议，理解为什么"单一 ADL 入口 + 标签类型"比"每个协议一个 ADL 函数名"更好。

## 前置理解

- **从 CPO 到 tag_invoke 的动机**：如果框架有几十个定制点，每个定制点都需要一个独立的 ADL 函数名，用户仍然面临名字冲突问题。
- **tag_invoke 的核心思想**：只暴露一个 ADL 函数名 `tag_invoke`，所有定制都通过第一个参数（tag 类型）来区分。
- **dispatch 链条**：`cpo(args...)` -> CPO 的 `operator()` -> `tag_invoke(cpo_tag, args...)` -> 用户的 `friend tag_invoke(tag, ...)` 实现。
- **为什么用 friend 函数**：ADL 能找到它，同时不污染外层命名空间。

## 必做任务

1. 实现最小 tag_invoke 函数（含 poison pill）。
2. 定义 `connect_t` 标签类型。
3. 构建 `connect` CPO。
4. 自定义 sender 通过 friend tag_invoke 提供实现。
5. 自定义 query：`get_scheduler_t` 标签和对应 CPO。
6. 验证全链路。
7. 在笔记中解释为什么一个 `tag_invoke` 名字 + N 个 tag 类型比 N 个独立 ADL 函数名更安全。

## 进阶任务

- 为 `tag_invoke` 添加 concept 约束。
- 为 `connect` CPO 添加返回类型约束。
- 实现第三个定制点 `start_t`，串联完整的 sender-receiver 三步走。

## 验收点

- 能手动走通 `connect(sender, receiver)` 的完整 dispatch 链。
- 能说明 `tag_invoke` 的 poison pill 为什么重要。
- 能说明 friend 函数为什么能被 ADL 发现。
- 能解释为什么 `stdexec` 的每个 CPO 都是 tag 类型。

## 对应官方参考

- P1895R0: tag_invoke: A general pattern for supporting customisable functions
- `stdexec` 源码中 `tag_invoke` 的使用
- `libunifex` 中 tag_invoke 的早期实现
