> 对应章节：模块 G — 自行实现视图 / G-2 my_transform_view + range_adaptor_closure 注入 operator|（P2387R3）

## 目标

实现一个带闭包 pipe 语法的 `my_transform_view<V, F>`，以及配套的 `my_transform_closure<F>` 和工厂对象 `my_transform`，验证 `vec | my_transform(sq)`、`vec | my_transform(sq) | my_transform(inc)`、`my_transform(sq) | my_transform(inc)` 三种用法全部工作。

---

## 前置理解

- **range_adaptor_closure（P2387R3）**：继承 `range_adaptor_closure<Derived>` 后，`Derived` 自动获得两个 `operator|`：`(range, closure) → closure(range)`，以及 `(closure, closure) → 新 closure`（先应用左再应用右）。你只需要在 `Derived` 里实现 `operator()(range)` 这一个接口。
- **iterator 的两套 trait**：`iterator_concept` 继承底层（C++20 concept 系统视角，上界 random_access）；`iterator_category` 保守处理（C++17 算法视角，invoke 结果是 prvalue 时最高 forward）。
- **closure 必须值持有 F**：closure 可能比原始函数对象活得长，不能存引用。iterator 存 `F*`（指向 view 的字段）而非 `F` 值，以避免函数对象在迭代器间重复拷贝，但需确保迭代器生命期在 view 之内。
- **reference 和 value_type**：`reference = invoke_result_t<F&, range_reference_t<V>>`；`value_type = remove_cvref_t<reference>`（不能是引用）。

---

## 必做任务

### 任务 1 — 实现 iterator 的 `operator*()`

```cpp
constexpr reference operator*() const {
    return std::invoke(*f_, *it_);
}
```

`f_` 是指向 view 字段的指针，`*f_` 是 F&，`*it_` 是底层引用。

### 任务 2 — 实现 `operator++()`

前置版：`++it_; return *this;`。后置版返回旧值副本。

### 任务 3 — 实现 bidirectional `operator--()`

```cpp
constexpr iterator& operator--()
    requires std::bidirectional_iterator<InnerIt>
{ --it_; return *this; }
```

`requires` 约束保护：底层不是 bidirectional 时此重载不参与推导，iterator 退化为 forward。

### 任务 4 — 实现 `my_transform_view::begin()`

```cpp
constexpr iterator begin() {
    return iterator{f_, std::ranges::begin(base_)};
}
```

把 view 的 `f_` 字段地址和底层 begin 一起传给 iterator 构造函数。

### 任务 5 — 实现 `end()` 两个重载

```cpp
// common_range 分支：end 与 begin 同类型
constexpr iterator end()
    requires std::ranges::common_range<V>
{
    return iterator{f_, std::ranges::end(base_)};
}

// 非 common_range 分支：直接透传底层 sentinel
constexpr auto end()
    requires (!std::ranges::common_range<V>)
{
    return std::ranges::end(base_);
}
```

### 任务 6 — 实现 `my_transform_closure::operator()(R&&)`

```cpp
template <std::ranges::viewable_range R>
    requires std::regular_invocable<F&, std::ranges::range_reference_t<R>>
constexpr auto operator()(R&& r) const {
    return my_transform_view{std::views::all(std::forward<R>(r)), f_};
}
```

注意 `views::all` 把 viewable_range 包装成 view（ref_view 或 owning_view）。

### 任务 7 — 实现 `my_transform_fn` 两个重载

单参数：`return my_transform_closure<F>{std::move(f)};`

双参数：`return my_transform_view{views::all(forward<R>(r)), move(f)};`

---

## 进阶任务

**A. iterator_concept 继承验证**

```cpp
// vector 底层 → random_access
static_assert(std::random_access_iterator<decltype(rv.begin())>);
// list 底层 → bidirectional，不是 random_access
static_assert(std::bidirectional_iterator<decltype(rl.begin())>);
static_assert(!std::random_access_iterator<decltype(rl.begin())>);
```

**B. const 迭代与 F const& 约束**

若要支持 `const my_transform_view` 的迭代，`begin()` 需要 const 重载，且要求 `regular_invocable<F const&, range_reference_t<V>>` 成立。

**C. closure-to-closure 组合类型验证**

```cpp
auto c3 = my_transform(sq) | my_transform(inc);
auto result = vec | c3;  // c3 仍然是有效 closure
```

---

## 验收点

- `vec | my_transform(sq)` 编译通过并得到正确结果。
- `vec | my_transform(sq) | my_transform(inc)` 链式管道正确。
- `my_transform(sq) | my_transform(inc)` 产生新 closure，可以再被 `|` 消费（P2387R3 保证）。
- iterator 的 `reference = invoke_result_t<F&, range_reference_t<V>>`，`value_type = remove_cvref_t<reference>`。
- 能解释 closure 为什么必须值持有 `F`（不能存引用）。

---

## 观察点

- `range_adaptor_closure<Derived>` 是 P2387R3 的正式化成果。在此之前，标准库内部用 `__range_adaptor_closure_t`（libstdc++ 命名）等私有实现，P2387R3 将其暴露给用户代码。这是从"使用 `|`"到"实现 `|`"的直接连接。
- iterator 的 `f_` 存的是 `F*`，而不是 `F` 值。这避免了函数对象在每个迭代器之间重复拷贝，但要求迭代器的生命期在 view 之内（view 被 move 后，已取出的迭代器会悬垂）。
- `iterator_category`（C++17 视角）和 `iterator_concept`（C++20 视角）的分裂是因为：C++17 前向迭代器要求 `*it` 是可绑定到 const 引用的左值，invoke 结果（prvalue）不满足；C++20 的 `forward_iterator` concept 不要求真实左值引用，所以 `iterator_concept` 可以继承底层。

---

## 常见坑

- **忘继承 `range_adaptor_closure`**：`vec | my_transform(sq)` 找不到 `operator|`，报"no match for operator|"，误以为是 view 定义问题。
- **iterator 存 F 引用而非指针**：closure 是临时对象，若 iterator 存闭包引用，在 `auto r = vec | my_transform(sq);` 后 closure 销毁，iterator 的引用悬垂。
- **`value_type` 未 decay**：`value_type` 如果写成 `invoke_result_t<...>` 而不是 `remove_cvref_t<invoke_result_t<...>>`，会包含引用，导致 std::iter_value_t 推导失败。
- **`operator|` 写成成员函数**：CPO 的 `operator|` 必须是非成员，`range_adaptor_closure` 提供的是 hidden friend，不需要你自己定义。

---

## 复盘问题

1. `range_adaptor_closure` 提供的两个 `operator|` 重载如何区分 `(range, closure)` 和 `(closure, closure)` 两种情况？
2. 如果要为 `my_filter_view` 也添加 pipe 支持，需要写哪几个类型？哪些可以复用？
3. closure 存储 `F` 的值意味着每次写 `my_transform(sq)` 都拷贝了 `sq` 一次。在性能敏感场景下你会怎么优化？
4. P2387R3 之前，标准库是怎么让 `views::transform(f) | views::filter(pred)` 工作的？

---

## 对应官方参考

- P2387R3（`range_adaptor_closure`：CRTP 基类正式化，`operator|` 语义）
- P0896R4（`transform_view` 原始设计）
- cppreference: `std::ranges::range_adaptor_closure`
- cppreference: `std::ranges::transform_view`（对照标准实现参考）
