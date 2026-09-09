# 练习 F-1：类型列表基础设施

## 目标

亲手实现 `type_list<Ts...>` 及其编译期操作（合并、去重、变换、过滤），建立"类型列表是 sender 框架的编译期容器"的直觉。

## 前置理解

- 你知道 C++ variadic template 的基本语法。
- 你知道 `static_assert` 可以在编译期检查条件。
- 本题的重点是编译期类型操作，不涉及运行期行为。

## 必做任务

1. 定义 `type_list<Ts...>`。
2. 实现 `concat<TypeList1, TypeList2>`：合并两个 type_list。
3. 实现 `unique<TypeList>`：去除重复类型。
4. 实现 `transform<TypeList, MetaFn>`：对每个类型应用元函数。
5. 实现 `filter<TypeList, Pred>`：保留满足谓词的类型。
6. 用这些工具模拟 `when_all` 的值类型合并。
7. 手动验证：用 `static_assert` + `std::is_same_v` 验证所有操作。
8. 所有验证通过 `static_assert`：编译成功就是验收。

## 进阶任务

- 实现 `size<TypeList>` 编译期常量。
- 实现 `at<TypeList, N>` 编译期索引。
- 实现 `flatten<TypeListOfTypeLists>`：展平嵌套 type_list。
- 用 `transform` + `filter` + `unique` 的组合来模拟更复杂的签名变换场景。

## 验收点

- 能实现 `concat`、`unique`、`transform`、`filter` 四个类型列表操作。
- 能用 `static_assert` 证明每个操作的正确性。
- 能用这些工具模拟 `when_all` 的值类型合并。
- 能说明为什么 sender 框架需要在编译期做这些类型列表操作。

## 观察点

- `type_list` 就是编译期的 `std::vector`——只不过它装的是类型，不是值。
- `concat` 就是编译期的 `push_back` / `append`。
- `transform` 就是编译期的 `std::transform`。
- `filter` 就是编译期的 `std::copy_if`。
- `unique` 就是编译期的去重。

## 对应官方参考

- `stdexec` 内部的 `__types` / `__type_list` 工具
- P2300 中 `completion_signatures` 的类型操作说明
- Boost.Mp11 或类似元编程库的设计思路
