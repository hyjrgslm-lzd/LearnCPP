# 练习 D-1：zip / zip_transform / adjacent / slide / chunk / stride

> 对应章节：../../06-模块D-C++23高阶视图与协程桥.md §练习 D-1
> 提案：P2321R2 / P2442R1 / P1899R3
> C++ 标准：C++23

## 目标

观察五个"改变 range 形状或组合形状"的 C++23 适配器，理解它们产出元素类型的差异，以及 `zip_view` 的 proxy reference 导致 iterator 双轨分离的原因。

## 前置理解

- 已读 `01-心智模型.md` 的 iterator_concept 双轨章节：`iterator_concept` 描述 C++20 能力，`iterator_category` 兼容 C++17 算法期望。
- 已完成模块 C1 练习 C1-1（join/join_with），理解嵌套 range 的展平与 iterator_concept 合成规则。
- 接受这题的重点是"形状变化"与"元素类型"，而不是具体数值输出。

## 必做任务方向

1. **zip 基础**：用 `views::zip(a, b, c)` 观察截短行为（长度 = min），打印各 tuple 元素。
2. **zip 双轨验证**：用 `static_assert` 同时验证 `iterator_concept == random_access_iterator_tag` 且 `iterator_category == input_iterator_tag`，说出原因（proxy reference）。
3. **zip_transform**：用 `zip_transform(plus, a, b)` 与等价的 `zip + transform` 对比写法。
4. **adjacent<N>**：对 `{1,2,3,4,5}` 分别用 `adjacent<2>` 和 `adjacent<3>`，用 `static_assert` 验证 `tuple_size == N`。
5. **slide(n) vs adjacent<N>**：对比产出类型（subrange vs tuple），验证 slide 不能结构化绑定。
6. **chunk(n)**：对 `{1..7}` 用 `chunk(3)`，观察最后一块不足 3 个不丢弃。
7. **stride(n)**：对 `{1..7}` 用 `stride(2)`，用 `static_assert` 验证 random_access_range。

## 进阶预计方向

1. 打印 `zip_view` 的 `iterator_concept` 与 `iterator_category`，亲眼确认双轨差异。
2. 用 `std::list<int>` 替换 `vector` 作为 `stride` 底层，验证迭代器不再是 random_access。
3. 用 `views::zip(a, views::repeat(0))` 模拟全零配对，对比 `zip_transform` 写法。
4. 验证 `zip` 不是笛卡尔积（笛卡尔积需要 `views::cartesian_product`，见模块 A 练习 A-3）。

## 验收点

- 三段基础任务代码编译通过，输出与头注释一致。
- 能用 `static_assert` 区分 `zip_view` 迭代器的 `iterator_concept`（random_access）与 `iterator_category`（input），并说出原因（proxy reference）。
- 能说出 `adjacent<N>` 与 `slide(n)` 的两个核心差异：静态 N vs 动态 n；产出 tuple vs 产出 subrange。
- 能解释 `stride(n)` 在 random_access 底层下 O(1) advance 的机制（`it += n * k`）。
- 能区分 `chunk(n)` 和 `slide(n)`：chunk 不重叠分块，slide 滑窗相邻重叠。

## 观察点

- `zip_view` 的 proxy reference 语义：解引用返回 `tuple<int&, double&, ...>`，这个 tuple 是临时值，不是真实左值引用。C++17 算法（如 `std::sort`）假设 `*it` 是左值引用以做 swap，因此 `iterator_category` 必须降为 `input_iterator_tag`。
- `adjacent<N>` 的零开销：N 是编译期常量，tuple 大小在编译期确定，可以完全内联和展开。`slide(n)` 的 subrange 是通用迭代器对，灵活但不能结构化绑定。
- `chunk_view` 的最后一块：`size(range) % n != 0` 时，最后一块包含剩余元素（小于 n 个），不丢弃。
- `stride_view` 的大小：`(size + n - 1) / n`（向上取整），底层是 sized_range 时本身也是 sized_range。

## 常见坑

- **把 zip 当笛卡尔积**：`views::zip(a, b)` 长度为 `min(|a|, |b|)`，不是 `|a| * |b|`。笛卡尔积用 `views::cartesian_product`。
- **把 adjacent<N> 当非重叠分块**：`adjacent<3>` 是滑窗，相邻窗口共享元素；`chunk(3)` 是非重叠分块。
- **对 non-random-access 底层用 stride 期望 O(1)**：`list | stride(2)` 的 `++it` 内部做 2 次前向推进，是 O(n)。
- **对 slide 结果做结构化绑定**：slide 产出 subrange，无法 `auto [a,b,c] = window`；结构化绑定请用 `adjacent<N>`。

## 提示

- 验证 `iterator_concept` 时用 `typename It::iterator_concept`；验证 `iterator_category` 时用 `typename It::iterator_category`。
- `zip_transform` 的第一个参数是可调用对象，其余是要组合的 ranges——顺序不要搞反。
- 测试 `stride` 的 O(1) 属性时，用 `static_assert(std::ranges::random_access_range<...>)` 而非运行时计时。

## 复盘问题

1. 为什么 `zip_view` 的 proxy reference 必须把 `iterator_category` 降为 `input_iterator_tag`，而 `iterator_concept` 仍然可以是 `random_access_iterator_tag`？（提示：两套系统的受众不同——C++17 算法 vs C++20 concept）
2. `adjacent<2>` 和 `zip(v, v | views::drop(1))` 在语义上是否等价？两者的元素类型有何差异？
3. 如果底层 range 是 `views::filter(v, pred)`（bidirectional），`stride(n)` 的迭代器概念是什么，`++it` 的代价是什么？
4. `chunk(n)` 产出的子 range 是 borrowed 的吗？当底层 vector 被销毁后，子 range 中的迭代器是否还有效？

## 对应官方参考

- P2321R2：C++23 `views::zip` 与 `views::zip_transform`（proxy reference 与 iterator_category 的设计讨论）
- P2442R1：C++23 `views::chunk`、`views::chunk_by`、`views::slide`
- P1899R3：C++23 `views::stride`
- cppreference：[`std::ranges::zip_view`](https://en.cppreference.com/w/cpp/ranges/zip_view)
- cppreference：[`std::ranges::adjacent_view`](https://en.cppreference.com/w/cpp/ranges/adjacent_view)
- cppreference：[`std::ranges::chunk_view`](https://en.cppreference.com/w/cpp/ranges/chunk_view)
- cppreference：[`std::ranges::slide_view`](https://en.cppreference.com/w/cpp/ranges/slide_view)
- cppreference：[`std::ranges::stride_view`](https://en.cppreference.com/w/cpp/ranges/stride_view)
