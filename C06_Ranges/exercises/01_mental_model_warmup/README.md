> 对应章节：../../01-心智模型.md §六个最重要的对象 + §三条 view 语义公理

## 目标

在写任何真正的管道代码之前，先用 `static_assert` 把心智模型里的六个核心对象
和三条语义公理钉牢。每一条断言都对应一个"你已经理解了这件事"的检查点。

## 前置理解

- 阅读 `01-心智模型.md` 中"六个最重要的对象"一节，理解
  range / view / iterator+sentinel / range_adaptor_closure / CPO / projection
  各自的定义和彼此的关系。
- 阅读"三条 view 语义公理"一节，理解为何 O(1) move/copy/destroy 是
  `std::ranges::view` concept 的语义要求，而不是性能建议。
- 阅读"借用（borrowed）与悬垂（dangling）"一节，理解 `dangling` 的编译期保护机制。

## 预计练习方向

- 用 `static_assert` 验证六个核心类型的 concept 归属（range / view / borrowed_range）。
- 构造无界 `iota_view`，验证 `end()` 类型是 `std::unreachable_sentinel_t`，
  begin/end 类型不同（非 `common_range`）。
- 演示 range adaptor closure 的 `|` 语法：`v | views::transform(f)` 与
  `views::transform(v, f)` 等价；两个 closure 可以预先用 `|` 组合。
- 验证 CPO（`ranges::begin`）是函数对象，可作为值传递；
  niebloid（`ranges::sort`）不被 ADL 劫持。
- 定义 `struct Person` 并用 `ranges::sort` + projection（成员指针）排序，
  对比 C++17 lambda 比较器写法。
- 通过"构造 → 移动 → 析构"的流程感受三条 view 语义公理：
  `string_view` 拷贝不复制字符数据；`vector<int>` 不是 view。
- 用函数返回临时 `vector` 传给 `ranges::find`，验证返回类型是 `ranges::dangling`。

## 进阶预计方向

- 检查 `iota_view` 迭代器的 `iterator_concept` 与 `iterator_category` 双轨，
  思考两者在 proxy reference 场景下为何会不同。
- 填写"六大 concept 对照表"（sized / common / view / borrowed / input / random_access），
  对 `vector`、有界 iota、无界 iota、filter_view 各填一行。
- 用 `enable_borrowed_range` 为自定义类型手动标记 borrowed，观察 `dangling` 保护消失。

## 验收点

- 六个核心对象对应的所有 `static_assert` 全部编译通过，无需注释任何一行。
- 能不看文档，口述三条 view 语义公理并举出一个"违反公理的类型"。
- 能解释为什么 `views::filter` 会把 random access 迭代器概念降到 bidirectional。
- 能演示 `ranges::dangling` 的编译期保护：解引用 dangling 触发编译错误。

## 观察点

- `std::vector<int>` 不是 view，根本原因是析构需要释放堆内存，代价与容量相关（O(n)）。
- `std::span<int>` 是 view 也是 borrowed_range：不拥有内存，迭代器有效性与 span 生命周期无关。
- `ranges::begin` 是函数对象实例，调用它时不参与 ADL，内部再决定走成员 `.begin()` 还是非成员 `begin()`。
- projection 在算法内部的应用顺序：先 `proj(e)`，再把结果传给 `comp` 或 `pred`。
- `filter_view::begin()` 是非 const 成员函数，因为它需要写入"第一个满足条件元素的位置"缓存。

## 常见坑

- 把 `std::ranges::view` 误理解为"轻量容器"——view 不负责任何元素的生命周期。
- 混淆"range"和"view"：所有 view 都是 range，但大多数 range（如 `vector`）不是 view。
- 以为 `ranges::sort` 是函数模板，实际上是函数对象（niebloid）——这个区别决定了
  它不会被 ADL 劫持，且可以作为一等值传递。
- 误认为"projection 就是语法糖"——projection 明确分离"映射步骤"与"谓词步骤"，
  使代码意图更清晰，并给编译器更好的内联机会。
- 期待 `ranges::find(get_vec(), 3)` 返回 `vector::iterator`，
  实际上 `vector` 不是 borrowed_range，右值传入时返回 `ranges::dangling`。

## 提示

- 验证 concept 优先用 `static_assert`，不要靠直觉猜测：编译器错误信息会指出确切的不满足原因。
- 验证 CPO 是函数对象：`auto cpo = std::ranges::begin;` 若能编译，就说明它是可复制的函数对象。
- 测试 `ranges::dangling`：
  ```cpp
  auto get_vec = []{ return std::vector<int>{3,1,4}; };
  auto it = std::ranges::find(get_vec(), 3);
  static_assert(std::same_as<decltype(it), std::ranges::dangling>);
  ```
- 测试 range adaptor closure 预组合：
  ```cpp
  auto pipeline = std::views::filter([](int x){ return x%2==0; })
                | std::views::transform([](int x){ return x*x; });
  // pipeline 本身是 range_adaptor_closure，等待一个 range
  ```

## 复盘问题

1. 为什么 `std::vector<int>` 不是 view，而 `std::span<int>` 是 view？（三条语义公理）
2. `ranges::sort` 是函数模板还是函数对象？为什么这个区别对 ADL 很重要？
3. projection 在算法内部的应用顺序是什么：先 projection 还是先 comp/pred？
4. 什么是 borrowed_range？为什么把右值 `vector` 传给 `ranges::find` 会返回 `dangling`？
5. `iterator_concept` 和 `iterator_category` 分别给谁看？proxy reference 迭代器
   为什么需要把 `iterator_category` 设为 `input_iterator_tag`？
6. range adaptor closure 的 `operator|` 满足结合律意味着什么？
   `r | a | b` 和 `r | (a | b)` 是等价的吗？

## 对应官方参考

- cppreference：[`std::ranges::range`](https://en.cppreference.com/w/cpp/ranges/range)
- cppreference：[`std::ranges::view`](https://en.cppreference.com/w/cpp/ranges/view)
- cppreference：[`std::ranges::borrowed_range`](https://en.cppreference.com/w/cpp/ranges/borrowed_range)
- cppreference：[`std::ranges::dangling`](https://en.cppreference.com/w/cpp/ranges/dangling)
- cppreference：[`std::ranges::range_adaptor_closure`](https://en.cppreference.com/w/cpp/ranges/range_adaptor_closure)
- cppreference：[`std::identity`](https://en.cppreference.com/w/cpp/utility/functional/identity)
## 参考解析

预测：`vector`、`string_view`、`array` 是 range，`int` 不是；`string_view`、`span`、`iota_view` 是 view，`vector` 不是 view。无界 `iota_view` 的 end 是 `unreachable_sentinel_t`，所以不是 common_range；右值 `vector` 传给返回迭代器的 ranges 算法得到 `ranges::dangling`，因为 borrowed_range 不负责延长所有者生命周期。

当前 `main.cpp` 的完整程序已经把这些预测写成 `static_assert` 和 `check`：它同时验证 span 迭代器仍指向原数组、`find` 对右值 vector 返回 dangling、对 borrowed `string_view` 返回真实迭代器。扩展时可以加入 `owning_view<vector<int>>`，观察 view 可以拥有元素，但 borrowed 语义仍不等于持有临时对象生命周期。
