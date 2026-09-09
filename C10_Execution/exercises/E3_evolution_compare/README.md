# 练习 E-3：演进对比与 Member-first Dispatch

## 目标

把 E-2 中基于 `tag_invoke` 的 sender 重构为 member-function dispatch（C++26 方向），然后横向比较四种定制机制，形成完整的演进认知。

## 前置理解

- **tag_invoke 的样板代码问题**：每个定制都需要写 `friend tag_invoke(SomeTag, MyType, Args...)`，可读性不算好，样板代码也不少。
- **Member-first dispatch（P2855 方向）**：C++26 的 `std::execution` 提案倾向于让 CPO 优先检测成员函数。
- **两种场景的区分**：
  - 你拥有类型的源码 -> 成员函数最自然
  - 你不拥有类型的源码 -> 必须用 `tag_invoke` 或其他外部注入方式
- **stdexec 的实际现状**：较新版本已经同时支持两种接入方式。

## 必做任务

1. 重构为成员函数版本。
2. 更新 CPO：优先检测成员函数，fallback 到 tag_invoke。
3. 保留两种类型做对照。
4. 写对照表（裸 ADL / CPO / tag_invoke / member-first）。
5. 讨论适用场景。
6. 写演进总结：ADL (C++98) -> CPO/Niebloid (C++20) -> tag_invoke (P1895) -> member-first (P2855/C++26)。

## 进阶任务

- 实现更完整的 member-first CPO（`set_value` 完整链路）。
- 研究 `stdexec` 源码中实际的 CPO dispatch 策略。
- 设计一个混合场景：member 方式的 `connect` + tag_invoke 方式的 `get_env`。

## 验收点

- member 版本 sender 能通过同一个 CPO 正常工作。
- CPO 能同时处理 member 和 tag_invoke 两种版本。
- 对照表覆盖至少 6 个维度。
- 能清楚说出 member-first 无法覆盖的场景。
- 能完整复述四阶段演进线索。

## 对应官方参考

- P2855R1: Member customization points for Senders and Receivers
- P1895R0: tag_invoke
- P2300R10: `std::execution` 提案中的 CPO 定义
- `stdexec` 源码中 CPO 的实际 dispatch 实现
