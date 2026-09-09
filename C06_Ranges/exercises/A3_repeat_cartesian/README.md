> 对应章节：../../02-模块A-视图工厂与惰性.md §练习 A-3

## 目标

用两个 C++23 新视图工厂感受"视图是蓝图，只有 for-range 才触发迭代"；
理解 `repeat_view` 的无界/有界两种形态；理解 `cartesian_product_view` 的
iterator_concept 由底层 range 决定的机制，以及空积语义（P2540R1）。

## 前置理解

- 完成练习 A-1，理解无界 view 与 `unreachable_sentinel_t` 的关系。
- 理解"视图是蓝图"：view 对象只记录"如何生成元素"的最小状态，
  迭代器推进时才真正产生元素值。
- 理解 `cartesian_product_view` 产出各底层 range 的笛卡尔积——每个元素是一个
  `tuple`，由各底层 range 当前元素组成。

## 预计练习方向

- 无界 `views::repeat(42)`：验证不满足 `sized_range` / `common_range`，
  满足 `random_access_range`；用 `views::take(5)` 限制后 for-range 打印。
- 有界 `views::repeat(42, 10)`：验证满足 `sized_range`；打印 size 和全部元素。
- `views::cartesian_product(nums, chars, flags)`：验证 `random_access_range` +
  `sized_range`；打印 size（3×2×2=12）和第一个元素。
- 空积 `views::cartesian_product()`：验证 size 为 **1**（不是 0）；
  for-range 打印一行"got empty tuple"；用 `static_assert` 验证元素类型是 `tuple<>`。

## 进阶预计方向

- 验证无界 `repeat(42)` 所有元素指向同一个对象
  （`&*it1 == &*std::next(it1)`）——view 只存储一个值，迭代器每次解引用返回该值引用。
- 用 `cartesian_product` 结合 `views::filter` 找出所有勾股数三元组（1..20），
  观察 iterator_concept 降级：`filter_view` 给出 bidirectional 上界。
- 理解含 `filter_view` 管道不能存为 `const` 变量（`begin()` 是非 const 成员）。

## 验收点

- 能证明无界 `repeat_view` 不满足 `sized_range`，有界形式满足 `sized_range`。
- 能证明 `cartesian_product_view` 的 `size()` 等于各维度 size 之积，
  并用 `static_assert` 验证 `sized_range` 属性。
- 能用运行结果证明"空积等于一个元素（空 tuple）而不是零个元素"（P2540R1 语义）。
- 能解释为什么 `cartesian_product | filter` 的结果至少是 `bidirectional_range`，
  但不期待它是 `random_access_range`（`filter_view` 给出了 bidirectional 的硬上界）。

## 观察点

- `repeat_view` 是"视图是蓝图"的极端案例：view 对象只存储一个值（`42`），
  迭代器每次解引用都返回该值的 const 引用，不需要额外存储。O(1) 构造，符合三条语义公理。
- `cartesian_product_view` 的元素不是预先存储的——迭代器内部维护各底层 iterator
  的组合状态，每次解引用才构造出 `tuple`。因此大小是各维度乘积，但构造仍是 O(1)。
- 空积语义（P2540R1）的直觉来自数学：零个集合的笛卡尔积是 {()}——恰好包含一个元素
  （空元组），而不是空集。这与 `empty_view` 的零个元素截然不同。
- `filter_view` 的 `iterator_concept` 上界是 `bidirectional_iterator`：底层若更强
  会被截顶，底层若更弱则透传（forward → forward，input → input）。

## 常见坑

- **无界 `repeat_view` 忘记 `take`**：`for (int x : std::views::repeat(42))` 是无限循环，
  程序不会终止。无界 view 必须配合 `take` / `take_while` / `zip` 等有限终止条件。
- **含 `filter_view` 的管道存为 `const`**：`begin()` 是非 const 成员（需要写入缓存），
  const 存储后无法迭代。解决方案：不要 const 存含 filter 的管道。
- **期待空积产出零个元素**：直觉上"没有任何底层 range，自然没有元素"，
  但数学上空积恰好是一个元素（空元组）。P2540R1 明确要求 `cartesian_product_view`
  在无参数调用时产出一个 `tuple<>` 元素。
- **`repeat_view` 元素类型误解**：`repeat(42)` 产出的是 `const int&`（对存储值的
  const 引用），而不是 `int` 值。所有元素都指向同一个对象。
- **`cartesian_product_view` 元素类型误解**：迭代器解引用产出的是
  `tuple<底层 range reference...>`，是引用的组合，不是拷贝的组合。

## 提示

- 验证空积 size：
  ```cpp
  static_assert(std::ranges::size(std::views::cartesian_product()) == 1);
  ```
- 验证 `repeat_view` 的引用语义：
  ```cpp
  auto r = std::views::repeat(42);
  auto it = r.begin();
  assert(&*it == &*std::next(it));  // 同一个 const int 对象
  ```
- 检查 iterator_concept 降级：经过 `filter_view` 后最高只有 `bidirectional_range`，
  不要对 `random_access_range` 做硬断言。

## 复盘问题

1. 为什么"空积等于一个元素"对于某些算法（如 `ranges::fold_left` 的初始值）
   在语义上是正确的，而不是"等于零个元素"？
2. `repeat_view` 满足 `random_access_range` 的意义是什么？
   `begin() + 100` 这样的跳转代价是 O(1) 吗？
3. 如果 `cartesian_product_view` 的某一底层 range 不满足 `forward_range`
   （例如 `istream_view`），整个积的 iterator_concept 会是什么？
4. 对比 `views::repeat(42, 10)` 和
   `views::iota(0, 10) | views::transform([](int){ return 42; })`：
   两者行为是否相同？性能特征是否相同？哪个表达意图更清晰？

## 对应官方参考

- P2474R2：`std::views::repeat`（无界与有界形式的设计）
- P2374R4：`std::views::cartesian_product`（多维积视图）
- P2540R1：`cartesian_product_view` 空积语义澄清（零参数调用产出一个 `tuple<>` 元素）
- cppreference：[`std::ranges::repeat_view`](https://en.cppreference.com/w/cpp/ranges/repeat_view)
- cppreference：[`std::ranges::cartesian_product_view`](https://en.cppreference.com/w/cpp/ranges/cartesian_product_view)
- cppreference：[`std::ranges::filter_view`](https://en.cppreference.com/w/cpp/ranges/filter_view)
