# 练习 F-3：sender/receiver Concepts 实现

## 目标

从零实现 sender 和 receiver 的 concept 约束，理解"concept 不是装饰性注释，而是 sender 图在编译期能自动检查类型匹配的基础"。

## 前置理解

- 你已经完成练习 F-1 和 F-2，理解类型列表操作和 completion_signatures。
- 你知道 C++20 concept 的基本语法：`template <typename T> concept MyConcept = requires(T t) { ... };`。
- 你在模块 D 中手写过 sender 和 receiver，知道它们需要满足哪些结构要求。
- 你接受本题的 concept 是简化版，不需要覆盖标准的所有细节。

## 必做任务

1. **实现 `my_sender<S>` concept**：检查 S 是否有 `sender_concept` 和 `completion_signatures` 类型别名。

2. **实现 `my_receiver<R>` concept**：检查 R 是否有 `receiver_concept` 类型别名，以及是否支持 `set_stopped()` 调用。

3. **实现 `my_receiver_of<R, Sigs>` concept**：检查 receiver R 是否能处理特定的 `completion_signatures` Sigs 中声明的每一种 completion（value、error、stopped）。

4. **用 `static_assert` 测试有效和无效的类型组合**，观察编译期诊断。

5. **实现 `my_sender_to<S, R>` concept**：检查 sender S 是否能连接到 receiver R（综合 my_sender + my_receiver + my_receiver_of 检查）。

## 进阶任务

- 实现更强的 `my_receiver_of`，能处理多个 value completion 签名。
- 给 concept 加上更好的诊断信息。
- 实现 `my_operation_state<O>` concept。
- 思考：如果 C++ 没有 concept，实现者会用什么替代方案？

## 验收点

- 你能实现 `my_sender`、`my_receiver`、`my_receiver_of`、`my_sender_to` 四个 concept。
- 你能用 `static_assert` 对合法和非法类型组合进行编译期检查。
- 你能说明每个 concept 检查了什么，以及为什么这些检查对 sender 图的正确性很重要。

## 观察点

- concept 让编译错误信息从"一堆模板展开失败"变成"类型 X 不满足 concept Y"。
- `my_receiver_of` 是最关键的 concept——它把 sender 的签名声明和 receiver 的能力绑定在一起。
- `my_sender_to` 是最终检查——它把 sender、receiver、signatures 三者的匹配关系一次性验证。
- 在标准库实现中，`connect(sender, receiver)` 内部会自动做这些 concept 检查。

## 对应官方参考

- P2300 中 `sender`、`receiver`、`receiver_of`、`sender_to` concept 的定义
- `stdexec` 中 concept 定义的源码位置
- cppreference 上 C++20 concepts 的语法参考
