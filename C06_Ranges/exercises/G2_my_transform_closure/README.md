> 对应章节：模块 G — 自行实现视图 / G-2 my_transform_view + range_adaptor_closure 注入 operator|（P2387R3）

## 目标

实现一个带闭包 pipe 语法的 `my_transform_view<V, F>`，以及配套的 `my_transform_closure<F>` 和工厂对象 `my_transform`，验证 `vec | my_transform(sq)`、`vec | my_transform(sq) | my_transform(inc)`、`my_transform(sq) | my_transform(inc)` 三种用法全部工作。

---

## 前置理解

- **range_adaptor_closure（P2387R3）**：继承 `range_adaptor_closure<Derived>` 后，`Derived` 自动获得两个 `operator|`：`(range, closure) → closure(range)`，以及 `(closure, closure) → 新 closure`（先应用左再应用右）。你只需要在 `Derived` 里实现 `operator()(range)` 这一个接口。
- **iterator 的两套 trait**：`iterator_concept` 继承底层（C++20 concept 系统视角，上界 random_access）；`iterator_category` 保守处理（C++17 算法视角，invoke 结果是 prvalue 时降到 input；只有真实左值引用且底层暴露 `iterator_category` 时才继承底层）。
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

### 任务 5 — 实现 `end()` 与 sentinel wrapper

`my_transform_view` 接受任意 `viewable_range`，所以不能只处理 common range。先在 view 内前置声明 `class sentinel;`，让 iterator 授权它读取底层迭代器：

```cpp
class sentinel;

class iterator {
    // ... current_ 保存底层 iterator
    friend class sentinel;
};

class sentinel {
    std::ranges::sentinel_t<V> last_{};
    bool equal(const iterator& it) const { return it.current_ == last_; }
public:
    sentinel() = default;
    explicit sentinel(std::ranges::sentinel_t<V> last) : last_(last) {}
    friend bool operator==(const iterator& it, const sentinel& s) { return s.equal(it); }
    friend bool operator==(const sentinel& s, const iterator& it) { return s.equal(it); }
};
```

`end()` 用一处 `if constexpr` 分支：

```cpp
auto end() {
    if constexpr (std::ranges::common_range<V>) {
        return iterator{function_, std::ranges::end(base_)};
    } else {
        return sentinel{std::ranges::end(base_)};
    }
}
```

直接返回底层 sentinel 是旧错误：wrapper iterator 不能自动和底层 sentinel 比较，`istream_view` 这种 input/non-common range 会立刻失败。

### 任务 6 — 实现 `my_transform_closure` 的 `const&` / `&&` 双重载

```cpp
template<std::ranges::viewable_range R>
    requires std::copy_constructible<F> &&
             std::regular_invocable<F&, std::ranges::range_reference_t<R>>
auto operator()(R&& range) const& {
    return my_transform_view{std::views::all(std::forward<R>(range)), function_};
}

template<std::ranges::viewable_range R>
    requires std::regular_invocable<F&, std::ranges::range_reference_t<R>>
auto operator()(R&& range) && {
    return my_transform_view{std::views::all(std::forward<R>(range)), std::move(function_)};
}
```

`const&` 重载只能在 `F` 可复制时参与，用于复用同一个 closure；`&&` 重载移动 callable，支持 `vec | my_transform(MoveOnlyAdd{5})` 这种 move-only callable。`views::all` 把 viewable_range 包装成 view（ref_view 或 owning_view）。

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
- `iterator_category`（C++17 视角）和 `iterator_concept`（C++20 视角）的分裂是因为：prvalue invoke 结果不是底层引用，C++17 iterator category 保守降到 input；C++20 的 `iterator_concept` 可以按底层能力继承。注意不能在 `conditional_t` 中无条件命名 `iterator_traits<InnerIt>::iterator_category`，`istream_view` 这类 input iterator 可能没有该 typedef。

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

## Author-validation layout

本题已迁移到统一四路径验证：`main.cpp` 消费 `c06_g2::my_transform` 和 `c06_g2::my_transform_view`。Reference 与 good 各自实现；bad 是安全运行时错误实现，故意在每次解引用时复制 callable，由独立的复制构造计数拒绝；Student 可编译但忽略 callable。

checker 覆盖直接调用和管道调用、closure 组合、stateful callable、move-only callable、random-access 能力继承、`operator[]`、`size()` 和 prvalue transform 结果。MSVC C++26 当前可支持继承 `std::ranges::range_adaptor_closure` 的自写 closure 与管道组合，本题保留这个真实路径。

## r3 non-common input 修正

`my_transform_view` 接受 `viewable_range`，因此必须支持 `std::views::istream<T>` 这类 input/non-common range。非 common 分支不能返回底层 raw sentinel，除非 wrapper iterator 能直接和它比较；本题使用自定义 sentinel wrapper 保存 `sentinel_t<V>`。iterator 构造时移动底层 iterator，避免 move-only input iterator 被意外拷贝。`iterator_category` 用受约束 helper 惰性暴露，底层没有 `iterator_category` 时降到 `input_iterator_tag`。
## 状态与regular_invocable的语义边界

stateful表示对象保存了偏移量等参数，不表示调用时可以修改自身并让同一输入产生不同结果。标准transform_view要求regular_invocable：调用保持相等性，且不修改函数对象和参数；编译器只检查表达式形式，不能替你验证这个语义要求。

本题Stateful的operator()是纯`value + delta`。复制构造只记录外部诊断计数；记录view构造完成后的计数，再验证解引用没有额外复制，并且重复遍历得到相同结果。更改原始callable的delta后，view仍用自己的副本。bad仍是每次解引用复制callable，但不再用违反标准前提的“累加调用次数改变返回值”来检验它。

一手依据：[regular_invocable](https://eel.is/c++draft/concept.regularinvocable)、[transform_view约束](https://eel.is/c++draft/range.transform.view)。修改前的最小语义复现是只调用普通函数即可让同一参数返回不同结果；不要把这种函数传给标准view运行，也不要用它判定标准`transform_view`语义。
