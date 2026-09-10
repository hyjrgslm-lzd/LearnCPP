> 对应章节：模块 G — 自行实现视图 / G-1 my_take_view — 最小 view_interface 实战

## 目标

实现一个最小的 `my_take_view<V>`，继承 `view_interface` 让 CRTP 注入默认成员，用 `static_assert` 验证它满足 `std::ranges::view` concept。

---

## 前置理解

- **view_interface 是脚手架**：它通过 CRTP 向下转型（`static_cast<Derived&>(*this)`）调用 `Derived::begin()` 和 `Derived::end()`，按条件注入 `empty()`、`front()`、`back()`、`operator[]`、`size()`、`data()`。它不存储任何数据，也不提供 `begin()`/`end()`，这两个必须由 `Derived` 自己实现。
- **enable_view opt-in**：继承 `view_interface` 的类型，只要满足 `movable` 且能被 range 消费，`enable_view<T>` 自动为 `true`（C++20 规定）。无需显式特化 `enable_view`。
- **begin()/end() 分支**：`random_access + sized` 底层时，`begin()` 透传底层 begin、`end()` 返回 `begin() + min(count, size)`；`sized` 非 random-access 底层时，`begin()` 返回截断后的 `counted_iterator`、`end()` 返回 `default_sentinel_t`；`unsized` 底层时，`begin()` 返回保存底层 sentinel 的 guarded iterator，比较时同时检查 count 和底层 end。
- **推导指引（CTAD）**：让 `my_take_view{vec, 3}` 可以用，无需写 `my_take_view<ref_view<vector<int>>>{vec, 3}`。指引在 `viewable_range R&&` 层面接受参数，用 `views::all_t<R>` 映射到 view 类型。

---

## 必做任务

### 任务 1 — 实现 `begin()`

```cpp
constexpr auto begin() {
    auto first = std::ranges::begin(base_);
    auto n = non_negative_count();
    if constexpr (std::ranges::random_access_range<V> &&
                  std::ranges::sized_range<V>) {
        return first;
    } else if constexpr (std::ranges::sized_range<V>) {
        return std::counted_iterator{first, std::min(n, distance_count())};
    } else {
        return guarded_iterator{first, std::ranges::end(base_), n};
    }
}
```

`begin()` 不能永远透传底层。非 random-access 路径的终止信息在 begin 侧：sized 底层用截断后的 counted iterator；unsized 底层用 guarded iterator 保存底层 sentinel。

### 任务 2 — 实现 `end()`（三条路径）

```cpp
constexpr auto end() {
    if constexpr (std::ranges::random_access_range<V> &&
                  std::ranges::sized_range<V>) {
        auto n = std::min(non_negative_count(), distance_count());
        return std::ranges::begin(base_) + n;
    } else {
        return std::default_sentinel;
    }
}
```

`guarded_iterator == default_sentinel` 的条件必须是 `remaining == 0 || current == last`。只用 `counted_iterator + default_sentinel` 处理 unsized 短 range 是错误方案：`count_` 大于底层长度时会继续递增已经到 end 的底层迭代器。
### 任务 3 — 实现 `size()`

```cpp
constexpr auto size()
    requires std::ranges::sized_range<V>
{
    auto clamped = count_ < 0
        ? std::ranges::range_difference_t<V>{0}
        : count_;
    auto sz = static_cast<std::ranges::range_difference_t<V>>(
                  std::ranges::size(base_));
    return static_cast<std::ranges::range_size_t<V>>(
               std::min(clamped, sz));
}
```

注意 `count_` 是有符号类型（`range_difference_t<V>`），直接强转为无符号的 `size_t` 时负值会变成巨大正数。先夹到 `[0, ∞)` 再取 min。

### 任务 4 — 写推导指引

```cpp
template <std::ranges::viewable_range R>
my_take_view(R&&, std::ranges::range_difference_t<R>)
    -> my_take_view<std::views::all_t<R>>;
```

没有推导指引时，`my_take_view(vec, 3)` 中 `vec` 是 `vector<int>&`（不是 view），模板参数 `V` 无法推导，编译失败。

---

## 进阶任务

**A. 分支行为可观察验证**

用 `std::list<int>` 作底层（bidirectional，sized，非 random_access），触发路径 B；再用单遍 input/non-common/unsized range 触发路径 C，验证短输入不会越界。

**B. view_interface 注入条件表**

| 成员 | 注入条件 |
|------|----------|
| `empty()` | `sized_range<D>` 或 `forward_range<D>` |
| `front()` | `forward_range<D>` |
| `back()` | `bidirectional_range<D>` 且 `common_range<D>` |
| `operator[](n)` | `random_access_range<D>` |
| `data()` | `contiguous_range<D>` |

**C. enable_view 的三条等价路径**

继承 `view_interface`（最常用）、继承 `view_base`（只满足 enable_view 不注入成员）、显式特化 `enable_view<T> = true`（无法修改基类时）。

---

## 验收点

- `static_assert(std::ranges::view<my_take_view<...>>)` 编译通过。
- `my_take_view{vec, 4}` 推导指引生效，无需显式模板参数。
- `random_access + sized` 底层时 `common_range` 且 `sized_range`；`sized` 非 random-access 底层时用截断 counted iterator；unsized 底层时 guarded iterator 同时检查 count 和底层 end。
- `view_interface` 注入的 `empty()` / `front()` / `back()` / `operator[]` 可以直接使用。
- 能解释 `view_interface` 为什么不提供 `begin()` / `end()`，以及 CRTP 的向下转型机制。

---

## 观察点

- `view_interface` 的核心实现模式：`bool empty() { return ranges::begin(derived()) == ranges::end(derived()); }`，其中 `derived()` 是 `static_cast<Derived&>(*this)`。所有逻辑依赖 `Derived::begin()` 和 `Derived::end()`，是纯语法糖。
- 路径 B 下 `begin()` 返回 `counted_iterator`；路径 C 下返回 guarded iterator。它们与路径 A 的裸底层迭代器类型不同，`decltype(tv.begin())` 会随底层 range 能力改变。
- 继承 `view_interface` 并不自动满足 view concept——你还需要满足 `movable`，且 `begin()`/`end()` 组成合法 range。

---

## 常见坑

- **路径 B 的 begin() 忘了包装 `counted_iterator`**：直接返回裸迭代器时，range-for 中 `it != default_sentinel` 无法编译，因为裸迭代器与 `default_sentinel_t` 没有 `operator==`。
- **`size()` 里 count_ 负值强转**：`count_ < 0` 时直接 `static_cast<size_t>(count_)` 会得到接近 `SIZE_MAX` 的巨大值，让 `size()` 返回虚假数字。
- **没写推导指引**：`my_take_view(vec, 3)` 推导失败，错误信息提示"V 不满足 view"——实际是根本没有推导到 `V`。
- **忘记继承 `view_interface`**：view 仍然满足 `range`，但 `empty()`/`front()` 等成员缺失，误以为需要全部手写。

---

## 复盘问题

1. `view_interface` 的 `size()` 注入条件是什么？和你自己提供的 `size()` 成员有优先级冲突吗？
2. 如果不继承 `view_interface` 但手写了所有等价成员，`enable_view` 需要怎么处理？
3. P2325R3 放宽了 view concept 的 `default_initializable` 要求之后，你的实现哪些地方可以简化？
4. 为什么 `view_interface` 注入的 `back()` 要求 `common_range`？如果 `end()` 类型是 sentinel，如何实现等价的 `back()`？

---

## 对应官方参考

- P0896R4（`view_interface` 的设计）
- P2325R3（view 不必 default_initializable：放宽后 view concept 语义更新）
- cppreference: `std::ranges::view_interface`
- cppreference: `std::ranges::view` concept

## Author-validation layout

本题已迁移到统一四路径验证：`checks/main.cpp` 消费 `c06_g1::my_take` 和 `c06_g1::my_take_view`。Reference 与 good 各自实现；bad 是安全运行时错误实现，故意接受负数；Student 可编译但会取错起点。

本题自写接口选择把负 `count` 统一抛 `std::invalid_argument`，这是教学接口的安全约束；`std::views::take` 本身仍以非负计数作为前提，不应混为一谈。checker 覆盖 sized random-access、sized non-random-access、input non-common unsized、`n` 大于底层长度、`front/back/operator[]/size`，并要求 unsized end 同时检查“剩余计数为 0 或底层已到 end”，不能用裸 `counted_iterator + default_sentinel` 越过短输入。
