> 对应章节：../../08-模块E-CPO与niebloid.md 练习 E-1：手写一个简化版 ranges::begin CPO

## 目标

亲手实现一个只负责"成员优先 + ADL fallback"的 `my_begin` CPO，通过三个测试场景（成员版本、ADL 版本、无 begin 路径）感受 CPO 如何控制查找策略，并与函数模板版本的 ADL 行为做对比。

## 前置理解

- **为什么变量不参与 ADL**：ADL（Argument-Dependent Lookup）的触发条件是"对函数名的非限定查找"。当 `begin` 是一个变量（对象），名字查找在第一步就找到了这个变量，不会进入 ADL 阶段。这是 CPO 屏蔽 ADL 污染的根本原因——不是魔法，而是语言名字查找规则的直接推论。
- **`inline constexpr` 变量**：C++17 引入 `inline` 变量，允许在头文件里定义全局变量而不违反 ODR。CPO 就是这样的变量——`inline` 允许多翻译单元共享同一定义，`constexpr` 允许在常量表达式上下文调用。
- **`if constexpr` + `requires` 表达式**：CPO 的三阶查找在编译期完成，每一阶是一个 concept 约束，不是运行时分支。满足约束则进入该路径；否则透明跳到下一阶。
- **命名空间边界**：`has_adl_begin` concept 必须定义在 `_my_begin_impl` 命名空间内。若放在 `my_ranges` 命名空间，`requires { begin(r); }` 的 ADL 会查找 `my_ranges` 本身，可能把 `my_begin` 的 `operator()` 计入候选，逻辑混乱。

## 必做任务

1. **定义 `has_member_begin` concept**：检测 `r.begin()` 合法且返回 `input_or_output_iterator`。注意参数是 `R&`（左值引用），不是 `R&&`。

2. **定义 `has_adl_begin` concept**：前提 `!has_member_begin<R>`，且 `begin(r)`（非限定查找，走 ADL）合法且返回 `input_or_output_iterator`。必须放在 `_my_begin_impl` 命名空间内。

3. **实现 `my_begin_fn` 函数对象**：提供两个 `operator()` 重载——
   - `requires has_member_begin<R>`：返回 `r.begin()`，传播 `noexcept`
   - `requires has_adl_begin<R>`：返回 `begin(r)`，传播 `noexcept`
   - 不提供第三个重载（无 begin 路径 → 调用点 SFINAE 失败）

4. **声明 CPO 本体**：`inline constexpr _my_begin_impl::my_begin_fn my_begin{};`。理解这一行是整套机制的枢纽：变量名 `my_begin` 不参与 ADL。

## 进阶任务

- **加 `borrowed_range` 约束**：在两个 `operator()` 上增加 `requires (std::is_lvalue_reference_v<R> || std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>)`。测试右值 `vector`（拒绝）和右值 `string_view`（通过）。
- **对比函数模板版本**：将 `my_begin` 改写为函数模板，演示 `using my_ranges_bad::my_begin; my_begin(r)` 的 ADL 污染问题，以及 `auto f = my_ranges_bad::my_begin` 的编译错误。

## 验收点

- 运行输出证明成员路径和 ADL 路径都正确工作。
- `static_assert(!requires { my_ranges::my_begin(nb); })` 编译通过，证明无 begin 的类型被编译期拒绝。
- 能说出 `has_adl_begin` 必须放在 `_my_begin_impl` 命名空间的原因。
- 能说出 `inline constexpr` 在此处各自的必要性。
- 能对比"CPO 赋值给 auto"与"函数模板取地址"的行为差异。

## 观察点

- `inline constexpr _begin_fn begin{};` 这一行把函数对象实例化为一个有名字的全局变量。名字查找找到变量而不是函数，ADL 就此被隔离。
- CPO 的三阶查找是编译期多态，不是运行时 if：每一阶都是 `requires` 约束，只有满足约束才进入该路径。
- 实际标准库的 `ranges::begin` 还处理数组类型（`array + 0`）、`[[nodiscard]]` 标记，以及右值非 borrowed_range 的拒绝逻辑，核心思路与本练习一致。

## 常见坑

- **把 `my_begin` 写成普通函数模板**：立刻失去 ADL 屏蔽效果，等于回到起点。
- **把 `has_adl_begin` 放在 `my_ranges` 命名空间**：`requires { begin(r); }` 的 ADL 会查找 `my_ranges`，逻辑混乱。
- **忘记 `!has_member_begin<R>` 前提**：两个重载都能匹配时产生歧义错误。
- **误以为 `using my_ranges::my_begin; begin(r)` 能找到 CPO**：`using` 引入的是变量名 `my_begin`，调用 `begin(r)` 走的是自由函数 ADL，与 CPO 无关。

## 复盘问题

- `std::ranges::begin` 是 `inline constexpr` 变量，它有"地址"吗？能对它取地址（`&std::ranges::begin`）吗？如果能，类型是什么？
- 假设有两个命名空间 `ns1` 和 `ns2`，各自有 `begin(T&)` 自由函数，参数类型 `T` 相同。CPO 内部做 ADL 查找时，会找到几个候选？
- CPO 的成员优先策略和 stdexec 的 member-first dispatch（P2855 方向）有何相似和不同？

## 对应官方参考

- P0896R4 §22.7（`std::ranges::begin` 规范）
- Eric Niebler 博文 "Customization Point Design in C++11 and Beyond"
- N4381: Suggested Design for Customization Points
- cppreference: [std::ranges::begin](https://en.cppreference.com/w/cpp/ranges/begin)
