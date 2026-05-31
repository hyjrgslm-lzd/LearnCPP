> 对应章节：模块 G — 自行实现视图 / G-3 my_enumerate_view + enable_borrowed_range 条件特化

## 目标

实现一个 enumerate view，产出 `(index, element)` pair，并根据底层 range 是否 `borrowed_range` 来决定自身是否 borrowed；配套实现 `my_enumerate_closure` 和 `my_enumerate_fn`，验证与 `views::transform` 的组合。

---

## 前置理解

- **enable_borrowed_range 的语义**：标记"迭代器有效性不依赖 view 对象的生命周期"。rvalue view 传给算法时，若未标记，算法返回 `dangling` 而非真正的迭代器。这是正确性承诺，不是性能优化。
- **条件特化模式**：`enable_borrowed_range<my_enumerate_view<V>> = enable_borrowed_range<V>` 把底层 V 的 borrowed 属性透传给 my_enumerate_view。iterator 内部的 `idx_` 是值拷贝，不依赖 view 对象——因此可以透传。
- **proxy pair 与双轨**：`operator*()` 返回 `pair<size_t, T&>`（prvalue），不是真实左值引用。因此 `iterator_category = input_iterator_tag`（C++17 约定：proxy reference 最高 input），而 `iterator_concept` 可以继承底层（C++20 放宽了对 `*it` 是左值引用的要求）。
- **iter_move 必要性**：proxy reference 迭代器必须定制 `iter_move`，否则 `std::move(*it)` 只移动了 pair 外壳，不移动底层元素（pair 内的 `T&` 不因此变成 `T&&`）。

---

## 必做任务

### 任务 1 — 实现 `operator*()`

```cpp
constexpr reference operator*() const {
    return reference{idx_, *it_};
}
```

`reference = pair<size_t, range_reference_t<V>>`。`idx_` 是值（不是 view 的 idx 字段引用），`*it_` 是底层引用。注意：不能写成 `pair<size_t&, T&>`，idx_ 是迭代器内部局部状态，返回对它的引用是悬垂引用。

### 任务 2 — 实现 `operator++()` 前置/后置

```cpp
constexpr iterator& operator++() {
    ++it_; ++idx_; return *this;
}
```

idx_ 和 it_ 同步推进。

### 任务 3 — 实现 bidirectional `operator--()`

```cpp
constexpr iterator& operator--()
    requires std::bidirectional_iterator<InnerIt>
{ --it_; --idx_; return *this; }
```

### 任务 4 — 实现 `iter_move` hidden friend

```cpp
friend constexpr auto iter_move(const iterator& it)
    noexcept(noexcept(std::ranges::iter_move(it.it_)))
{
    return std::pair<std::size_t,
                     std::ranges::range_rvalue_reference_t<V>>{
        it.idx_,
        std::ranges::iter_move(it.it_)
    };
}
```

必须是 hidden friend（ADL 可找到），不能是普通成员函数——`ranges::iter_move` CPO 不走成员函数查找路径。

### 任务 5 — 实现 `begin()`

```cpp
constexpr iterator begin() {
    return iterator{std::ranges::begin(base_), 0};
}
```

从索引 0 开始。

### 任务 6 — 实现 `end()` 两个重载

common_range 分支：需要 end-index。用 `ranges::distance(base_)`——此处只进入 common_range 分支，若底层还是 sized_range，distance 是 O(1)；若底层仅是 common_range 而非 sized，distance 是 O(n)（慎用）。

非 common_range 分支：直接返回 `ranges::end(base_)`。

### 任务 7 — 实现 `enable_borrowed_range` 条件特化

```cpp
template <std::ranges::view V>
inline constexpr bool
    std::ranges::enable_borrowed_range<my_enumerate_view<V>> =
        std::ranges::enable_borrowed_range<V>;
```

特化必须在 `my_enumerate_view` 定义之后、`main` 使用之前出现。

### 任务 8 — 实现 `my_enumerate_closure::operator()(R&&)`

```cpp
template <std::ranges::viewable_range R>
constexpr auto operator()(R&& r) const {
    return my_enumerate_view{std::views::all(std::forward<R>(r))};
}
```

---

## 进阶任务

**A. iter_move 与 ranges::sort 的交互**

尝试对 enumerate view 调用 `ranges::sort`，分析为什么不合理（排序 (index, value) pair 时 index 会随 value 一起移动，语义上 index 应当是序号而非被排序的键）。

**B. 与 C++26 views::enumerate（P2164）的对比**

| 特性 | my_enumerate_view | C++26 views::enumerate |
|------|-------------------|------------------------|
| index 类型 | size_t | range_difference_t<V>（有符号） |
| reference 类型 | pair<size_t, T&> | enumerate_result<W, T&>（具名） |
| borrowed_range 标记 | 条件特化（本实现有） | 条件特化（标准同样有） |

**C. index 类型的陷阱**

本实现用 `size_t`（无符号），对大 range 无溢出问题。标准 P2164 用 `range_difference_t<V>`（有符号），对负偏移随机访问语义更清晰。把 `idx_` 改为有符号类型，观察 `operator+=` 里的类型转换变化。

---

## 验收点

- `my_enumerate_view` 迭代产出正确的 `(index, value)` pair。
- `enable_borrowed_range` 条件特化正确：`string_view` 底层时为 `true`，`vector` 底层时为 `false`。
- 与 `views::transform` 组合工作（验证 range_adaptor_closure 互操作性）。
- 能解释 `iterator_concept` 为什么可以继承底层而 `iterator_category` 必须是 `input_iterator_tag`。
- 能解释 `iter_move` 为什么对 proxy reference 迭代器是必要的。

---

## 观察点

- `enable_borrowed_range` 条件特化是库代码里非常常见的"透传 borrowed 属性"惯用法。无条件特化（`= true`）适用于"迭代器永远不依赖对象"的 span 类型；条件特化才是实现层的真实用法——根据底层 V 的 borrowed 属性决定自身是否 borrowed。
- `iterator_category = input_iterator_tag` 而 `iterator_concept = random_access_iterator_tag` 是"双轨"典型案例：proxy reference 导致 C++17 算法把该迭代器视为 input，但 C++20 concept 系统承认它的真实随机访问能力。
- `end()` 对 common_range 底层调用 `ranges::distance(base_)` 来计算 end-index。若底层是 sized_range 则 O(1)；若底层只是 common_range 而非 sized_range，则 O(n)——每次调用 `end()` 都遍历一遍底层。需要根据情况加 `requires sized_range<V>` 约束或改变实现策略。

---

## 常见坑

- **忘了 `enable_borrowed_range` 条件特化**：把 `my_enumerate_view<string_view>` 的右值传给算法时返回 `dangling`，而非真正的迭代器。没有编译错误，只是行为不符合预期。
- **`reference` 写成 `pair<size_t&, T&>`**：`idx_` 是迭代器内部局部状态，`operator*` 返回后就是悬垂引用。正确写法是 `pair<size_t, T&>`：index 是值，element 是引用。
- **end() 对非 sized_range 调用 distance**：退化成 O(n) 的重复代价，每次 `end()` 调用都遍历底层序列。
- **iter_move 写成普通成员函数**：CPO 不走成员函数路径，定制完全无效，回退到默认的 `std::move(*it)` 行为。

---

## 复盘问题

1. `enable_borrowed_range` 条件特化和无条件特化的使用场景分别是什么？
2. 为什么 `pair<size_t, T&>` 是 proxy reference，而 `pair<size_t&, T&>` 是错的？
3. 如果不定义 `iter_move`，`ranges::iter_move` 会回退到什么行为？对 proxy reference 迭代器这个回退是否安全？
4. C++26 的 `views::enumerate`（P2164）和你的实现有哪些核心差距？哪些差距会在实际使用中造成问题？
5. 对 `my_enumerate_view<owning_view<vector<int>>>` 调用 `ranges::find` 返回什么类型？为什么？

---

## 对应官方参考

- P1739R4（borrowed_range：迭代器安全返回的细化）
- P2164（C++26 `views::enumerate` 设计，作为标准对照）
- cppreference: `std::ranges::enable_borrowed_range`
- cppreference: `std::ranges::borrowed_range` concept
