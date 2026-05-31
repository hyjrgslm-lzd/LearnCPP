> 对应章节：模块 H — 高级实现模式 / H-2 common_iterator + iter_move/iter_swap 定制

## 目标

理解 `common_iterator<I, S>` 如何把"begin/end 类型不同"的 view 对接 C++17 遗留算法，并深入演示 `iter_move` / `iter_swap` CPO 对 proxy iterator 的必要性——没有这两个 CPO，`ranges::sort` 等依赖正确 move/swap 语义的算法会静默错误。

---

## 前置理解

- **common_iterator 的作用**：C++17 算法（`std::accumulate`、`std::copy` 等）要求 begin/end 是相同类型。C++20 的许多 view（如 `iota | take`）的 begin/end 类型不同（有 sentinel）。`common_iterator<I, S>` 内部持有 `variant<I, S>`，把两者统一为同一类型，作为 C++20 与 C++17 之间的桥梁。
- **proxy reference 的问题**：proxy iterator 的 `operator*()` 返回 prvalue（如 `tuple<T1&, T2&>`），不是真实左值引用。`std::move(*it)` 只移动 proxy 外壳，`tuple` 内的引用不变，元素级 move 不发生。
- **iter_move CPO**：`std::ranges::iter_move(it)` 通过 ADL 找到 hidden friend `iter_move`，或回退到 `std::move(*it)`。对 proxy iterator 必须定制，返回 `tuple<T1&&, T2&&>`，才能触发真正的元素级 move。
- **iter_swap CPO**：`std::ranges::iter_swap(a, b)` 默认行为是 `std::ranges::swap(*a, *b)`。对 proxy tuple，这只交换 proxy 对象（引用不可重绑），语义错误。必须定制为逐维 swap 底层元素。
- **hidden friend 约束**：`iter_move` / `iter_swap` 必须是 hidden friend 或同命名空间非成员函数，才能被 ADL 找到。写成普通成员函数后 CPO 不走成员查找路径，定制完全无效。

---

## 必做任务

### 任务 1 — 理解 views::common 对接 C++17 算法

```cpp
auto r = std::views::iota(1) | std::views::take(10);
static_assert(!std::ranges::common_range<decltype(r)>);

auto cr = r | std::views::common;
static_assert(std::ranges::common_range<decltype(cr)>);
int sum = std::accumulate(cr.begin(), cr.end(), 0);  // 55
```

`views::common` 在底层使用 `std::common_iterator`。若底层已经是 common_range，`views::common` 直接透传不包装（性能优化）。

### 任务 2 — 实现 my_common_iterator::operator*()

```cpp
constexpr decltype(auto) operator*() const {
    return *std::get<I>(var_);
}
```

只有当 `var_` 持有 `I`（begin 端）时解引用才有意义；持有 `S`（end 端）时调用是 UB。

### 任务 3 — 实现 operator++()

```cpp
constexpr my_common_iterator& operator++() {
    ++std::get<I>(var_);
    return *this;
}
```

推进 `I`，`S` 不变（end 端不推进）。

### 任务 4 — 实现 operator==

```cpp
friend constexpr bool operator==(const my_common_iterator& a,
                                 const my_common_iterator& b) {
    if (a.var_.index() == 1 && b.var_.index() == 1) return true;   // (S,S)
    if (a.var_.index() == 0 && b.var_.index() == 1)
        return std::get<I>(a.var_) == std::get<S>(b.var_);          // (I,S)
    if (a.var_.index() == 1 && b.var_.index() == 0)
        return std::get<I>(b.var_) == std::get<S>(a.var_);          // (S,I)
    return false;  // (I,I) — common_range 使用场景中通常不出现
}
```

### 任务 5 — 实现 ZipIterator::operator*()

```cpp
reference operator*() const {
    return reference(*it1_, *it2_);
}
```

返回 `tuple<T1&, T2&>`（proxy pair）。

### 任务 6 — 实现 iter_move hidden friend

```cpp
friend auto iter_move(const ZipIterator& it) {
    return std::tuple<std::iter_rvalue_reference_t<It1>,
                      std::iter_rvalue_reference_t<It2>>(
        std::ranges::iter_move(it.it1_),
        std::ranges::iter_move(it.it2_)
    );
}
```

返回 `tuple<T1&&, T2&&>`，触发每一维的真正 move 语义。

### 任务 7 — 实现 iter_swap hidden friend

```cpp
friend void iter_swap(const ZipIterator& a, const ZipIterator& b) {
    std::ranges::iter_swap(a.it1_, b.it1_);
    std::ranges::iter_swap(a.it2_, b.it2_);
}
```

逐维 swap 底层元素，不依赖 proxy 的 swap（引用不可重绑）。

---

## 进阶任务

- 实现完整的 `my_common_iterator<I, S>`，验证它能被 `std::accumulate` 消费。
- 测试"移除 `iter_move` 定制后 `ranges::sort` 的行为"：注释掉 `iter_move` friend，观察编译错误或运行时只有 keys 被排序但 values 顺序不变的静默错误。
- 给 `ZipIterator` 加上 `iter_move` 的单元测试：用 `static_assert` 检查返回类型是 `tuple<T1&&, T2&&>` 而非 `tuple<T1&, T2&>`。
- 探索 `std::views::zip` 的标准实现（C++23），找到 libstdc++ 或 MSVC STL 中 `zip_view` 的 `iter_move` / `iter_swap` 实现，对比手写版本。

---

## 验收点

- `views::iota(1) | views::take(10) | views::common` 能成功传给 `std::accumulate`，结果为 55。
- 不加 `views::common` 时，直接传给 `std::accumulate` 编译失败（类型不匹配）。
- `ZipIterator` 的 `iter_move` 返回 `tuple<T1&&, T2&&>`（`static_assert` 验证类型正确）。
- `ranges::sort` 通过 `ZipIterator` 排序后，keys 和 values 同时被正确排列。
- 能解释为什么 `iter_move` / `iter_swap` 必须是 hidden friend 而不是普通成员函数。

---

## 观察点

- `iter_move(it)` 和 `iter_swap(a, b)` 是 ranges 算法与 iterator 交互的两个最底层 CPO。`sortable` concept 内部约束了这两个 CPO——不正确定制会直接导致 `sortable` 不满足，`ranges::sort` 无法编译。这是编译期安全保障。
- `std::sort`（老式算法）用 `std::swap(*a, *b)` 和直接赋值，对 proxy reference 的行为依赖实现，不可靠。这正是"ranges 算法 vs C++17 算法"在 proxy iterator 场景下的关键差异。
- `common_iterator` 的代价：每次 `operator==` 需要检查 variant 状态，比原生 iterator 稍慢；但这个代价只出现在需要传给 C++17 算法的场合，不影响 ranges 管道内部的性能。

---

## 常见坑

- **把 `iter_move` 写成普通成员函数**：`ranges::iter_move` CPO 通过 ADL 找到 hidden friend，不走成员函数路径。写成成员函数后 CPO 回退到默认行为 `std::move(*it)`，定制完全无效。
- **`iter_swap` 只 swap 了底层迭代器之一**：tuple 里每一维的引用都必须被 swap，漏掉任何一维会导致"部分排序"的静默错误。
- **忘记 `common_iterator` 在构造为 end 时存储 S 而不是 I**：若把 end 也构造为持有 I 的状态，`operator==` 永远不会在正确位置停止。
- **把 `views::common` 与 `views::as_const` 混淆**：前者解决"begin/end 类型不同"，后者解决"元素可变/不可变"——两个完全正交的问题。

---

## 复盘问题

1. `common_iterator` 的 `operator==` 需要比较 `variant<I, S>` 的两种状态，为什么"两个 I 相等"这种情况通常不需要处理？
2. 如果 `iter_move(it)` 没有被定制，`ranges::sort` 在排序 `ZipIterator` 范围时最可能出现什么现象？
3. 为什么 `ZipIterator` 的 `iterator_category` 必须设为 `input_iterator_tag`，即使它支持随机访问？
4. `std::views::common` 在底层使用了 `std::common_iterator`——如果底层 range 已经是 common_range，`views::common` 做什么优化？
5. 除了 `iter_move` 和 `iter_swap`，proxy iterator 还有哪个关键 CPO 需要定制？

---

## 对应官方参考

- P0896R4 §common_iterator 设计说明
- P2321R2 §zip_view 迭代器的 iter_move / iter_swap 定制
- P2494R2（ranges::as_const / ranges::as_rvalue 细化）
- cppreference: `std::common_iterator`、`std::ranges::iter_move`、`std::ranges::iter_swap`
