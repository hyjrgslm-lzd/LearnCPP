# 练习 F-2：completion_signatures 定义与变换

## 目标

亲手定义和变换 `completion_signatures`，理解 sender adaptor 是如何在编译期从输入签名推导出输出签名的。这是 sender 可组合性的类型级核心。

## 前置理解

- 你已经完成练习 F-1，能够操作类型列表。
- 你知道 sender 需要声明 `completion_signatures`（模块 D 练习 12 中已接触过）。
- 你理解 `completion_signatures` 描述的是"这个 sender 可能以哪些方式完成"。
- 你接受这题先用自己的简化版实现，不需要完全对齐标准库的所有细节。

## 必做任务

1. **定义简化版 `completion_signatures`**：先定义 `set_value_t`、`set_error_t`、`set_stopped_t` 三种标签类型，然后定义 `completion_signatures<Sigs...>`。

2. **实现 `value_types_of<Sigs>`**：从 `completion_signatures` 中提取所有 `set_value_t<...>` 签名。

3. **实现 `error_types_of<Sigs>`**：从 `completion_signatures` 中提取所有 `set_error_t<...>` 签名。

4. **实现 `sends_stopped<Sigs>`** 编译期谓词：检查 `completion_signatures` 中是否包含 `set_stopped_t`。

5. **实现简化版 `make_completion_signatures`**：给定输入 completion_signatures 和一个类型变换规则，计算输出 completion_signatures。核心思路：取出输入签名中的 value types，对每个应用变换规则，保留 error types 和 stopped，组装成新的 completion_signatures。

6. **用它模拟 `then(f)` 的签名推导**：假设 `f: int -> std::string`，输入签名中的 `set_value_t<int>` 应变成 `set_value_t<std::string>`，其余不变。用 `static_assert` 验证。

7. **处理 `f` 返回 `void` 的情况**：输出签名中的 value completion 应变成 `set_value_t<>`（无参数的 set_value）。

## 进阶任务

- 实现对多个 value completion 的变换，最终用 `unique` 去重。
- 尝试模拟 `let_value` 的签名推导。
- 实现 `add_error_type<Sigs, NewErr>`。
- 思考为什么标准中 `make_completion_signatures` 还需要接受 environment 参数。

## 验收点

- 你能从 `completion_signatures` 中提取 value types、error types，并检测 stopped。
- 你能实现简化版 `make_completion_signatures`，用它推导 `then(f)` 的输出签名。
- 你能处理 `f` 返回 `void` 的特殊情况。
- 所有验证通过 `static_assert`，编译成功即验收。

## 观察点

- `completion_signatures` 是 sender 的"类型级合同"——它告诉下游："我可能以这些方式完成"。
- `make_completion_signatures` 是 sender adaptor 的"类型级核心"——每个 adaptor 本质上都在做"给定上游签名，算出我的输出签名"。
- `then(f)` 只变换 value channel 的类型，不改变 error 和 stopped。
- 当你理解了签名变换，就能理解为什么组合器能在编译期做完类型检查——一切都是类型级别的函数组合。

## 对应官方参考

- P2300 中 `completion_signatures` 的定义
- P2300 中 `make_completion_signatures` 的规范
- `stdexec` 中 `then` adaptor 如何使用签名变换
