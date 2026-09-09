# 练习 E-1：CPO 与 Niebloid

## 目标

实现一个最小 CPO（Customization Point Object）和 Niebloid，与裸 ADL 自由函数方案做对照，亲手触发并修复 ADL 劫持问题。

## 前置理解

- **ADL（Argument-Dependent Lookup）**：C++ 在调用一个未限定函数名时，会根据实参类型所在的命名空间去查找候选函数。
- **ADL 劫持问题**：如果某个无关的第三方库恰好也定义了同名函数并且参数类型能匹配，编译器就可能选中那个无关的重载。
- **CPO（Customization Point Object）**：把定制点做成一个全局 `constexpr` 对象，因为对象不参与 ADL，所以不会触发非预期的 ADL 查找。
- **Niebloid**：C++20 Ranges 库中对 CPO 的一种叫法。`std::ranges::sort` 就是一个 Niebloid。
- **为什么 `std::execution` 的定制点是对象**：`stdexec::connect`、`stdexec::start`、`stdexec::set_value` 等全部是 CPO。

## 必做任务

1. 裸 ADL 版本：在 `lib_a` 和 `lib_b` 中各定义类型和 ADL 自由函数 `greet`。
2. 触发 ADL 劫持：新增 `third_party` 命名空间，演示劫持现象。
3. CPO 版本：定义 `constexpr` 函数对象，按优先级尝试 member -> ADL -> default。
4. 验证 CPO 防劫持。
5. 解释为什么 CPO 必须是对象。

## 进阶任务

- 让 CPO 的检测逻辑使用 C++20 concepts 或 `requires` 表达式来做 SFINAE。
- 实现一个返回值类型不同的 CPO：例如 `name_of` CPO。
- 尝试在 CPO 中加入 `static_assert` 或 `concept` 约束。

## 验收点

- 能用编译器输出或运行结果证明裸 ADL 版本存在劫持风险。
- 能用同样的测试场景证明 CPO 版本不受劫持。
- 能说出 CPO 内部的优先级链（member -> ADL -> default）。
- 能解释为什么 `std::execution` 中 `connect`、`start` 等都是对象而不是函数。

## 对应官方参考

- N4381: Suggested Design for Customization Points
- Eric Niebler 的博文 "Customization Point Design in C++11 and Beyond"
- `std::ranges` 中 niebloid 的设计
