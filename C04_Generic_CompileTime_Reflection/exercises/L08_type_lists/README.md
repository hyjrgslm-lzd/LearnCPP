# 练习 L08：type_list 与完成签名变换

先读 `../../chapters/08-type-lists.md`。只编辑 `src/student/type_list_tools.hpp`。

## Part 1：基础列表算法

实现 `type_list<Ts...>`、`map_t<F, List>`、`filter_t<P, List>`、`concat_t<A, B>` 和 `unique_t<List>`。

解析：`map` 对 `type_list<Ts...>` 做偏特化，再展开 `F<Ts>...`。`filter` 用空列表基例和 `T, Rest...` 递归步；先算 tail，再根据 `P<T>::value` 选择 `push_front<T, tail>` 或 tail。`unique` 用 `Seen/Rest` 不变量：`Seen` 按原顺序保存已接受类型，`Rest` 保存未扫描后缀；遇到重复类型时只推进 `Rest`，不改 `Seen`。

手推例子：`unique_t<type_list<int, double, int, char>>` 应得到 `type_list<int, double, char>`。第一次 `int` 进入 `Seen`，第二次 `int` 被跳过。

## Part 2：完成签名标签

定义 `value_sig<Ts...>`、`error_sig<E>`、`stopped_sig`。它们只是类型标签，不运行任何 sender。

解析：这是 C10 完成通道的类型层预习。不要添加 operation state、scheduler 或运行时模拟。

## Part 3：值签名变换

实现 `transform_completion_signatures_t<F, List>`。`value_sig<Ts...>` 按 `std::invoke_result_t<F, Ts...>` 变换；结果是 `void` 时变成 `value_sig<>`。`error_sig` 和 `stopped_sig` 保留。重复输出合并。

解析：不可调用的 `value_sig<Ts...>` 要让 `transformable<F, List>` 为 `false`，而不是让头文件无法包含。

## Part 4：惰性实例化

实现 `lazy_type_t<ChooseThen, ThenProvider, ElseProvider>`。provider 是带 `using type = ...` 的类型。`ChooseThen == true` 时只取 `ThenProvider::type`，不实例化 `ElseProvider::type`。

解析：checker 会用没有 `type` 的 `ExplosiveProvider` 做未选分支。diagnostic case 会单独编译 eager 反例，证明同时访问两边会在预期边界失败。
