> 对应章节：模块 H — 高级实现模式 / H-1 __non_propagating_cache + my_filter_view 缓存

## 目标

把模块 G 的 `my_filter_view` 升级，让它模拟标准 `filter_view` 的缓存行为。核心目标是亲手实现 `non_propagating_cache<T>`，并用它缓存 `begin()` 扫描结果，同时通过 `static_assert` 验证拷贝后的新 view 缓存为空——这是 view O(1) copy 公理的实现级保障。

---

## 前置理解

- **view O(1) copy 公理**：若 view 允许拷贝，拷贝代价必须是 O(1)。`filter_view::begin()` 需要 O(n) 扫描，结果被缓存。但"O(1) copy"的真正约束不是性能——而是"两个独立 view 对象必须有独立的迭代状态"。缓存若传播，两个对象共享同一个"已扫描"的状态，违反值语义。
- **non_propagating_cache 的语义核心**：拷贝/移动构造和赋值时清空缓存（`slot_` 不传播），强制新 view 重新扫描。这与普通 `optional<T>` 的拷贝语义（传播值）相反，正是它存在的原因。
- **begin() 必须是非 const 成员**：因为 `begin()` 需要写入 `cached_begin_`，而 C++ 中 `const` 成员函数不能修改非 `mutable` 数据成员。`filter_view::begin()` 非 const 是有意为之的设计约束，不是实现缺陷。
- **mutable optional 的陷阱**：若用 `mutable optional<It> cached_begin_`，`begin()` 可以变为 `const`，但拷贝时 optional 随值一起复制——两个 view 对象拿到同一个"缓存位置"的副本，仍然违反"独立迭代状态"的值语义。

---

## 必做任务

### 任务 1 — 实现 non_propagating_cache 拷贝/移动构造

```cpp
non_propagating_cache(const non_propagating_cache&) noexcept : slot_() {}
non_propagating_cache(non_propagating_cache&&) noexcept : slot_() {}
```

不论原对象的缓存状态如何，新对象的缓存始终为空。拷贝构造初始化为空（而非传播 `slot_`）是关键所在。

### 任务 2 — 实现拷贝/移动赋值

```cpp
non_propagating_cache& operator=(const non_propagating_cache&) noexcept {
    slot_.reset();
    return *this;
}
non_propagating_cache& operator=(non_propagating_cache&&) noexcept {
    slot_.reset();
    return *this;
}
```

赋值目标的缓存被清空（不继承源的缓存）。

### 任务 3 — 实现工具接口

```cpp
bool has_value() const noexcept { return slot_.has_value(); }
T& operator*() noexcept { return *slot_; }
const T& operator*() const noexcept { return *slot_; }
template<class... Args>
T& emplace(Args&&... args) {
    slot_.emplace(std::forward<Args>(args)...);
    return *slot_;
}
void reset() noexcept { slot_.reset(); }
```

### 任务 4 — 实现 my_filter_view::begin()（非 const）

```cpp
constexpr auto begin() {
    if (!cached_begin_.has_value()) {
        auto it  = std::ranges::begin(base_);
        auto end = std::ranges::end(base_);
        while (it != end && !std::invoke(pred_, *it)) {
            ++it;
        }
        cached_begin_.emplace(it);
    }
    return *cached_begin_;
}
```

第一次调用：O(n) 扫描，结果缓存。第二次调用：直接返回缓存，O(1)。

### 任务 5 — 实现 iterator::operator++()

```cpp
constexpr iterator& operator++() {
    ++current_;
    skip_bad();  // 跳过不满足谓词的元素
    return *this;
}
```

推进后立即调用 `skip_bad()`，保证迭代器始终指向满足谓词的位置（或末尾）。

### 任务 6 — 实现 bidirectional operator--()

```cpp
constexpr iterator& operator--()
    requires std::ranges::bidirectional_range<V>
{
    auto begin = std::ranges::begin(parent_->base_);
    do { --current_; }
    while (current_ != begin && !std::invoke(parent_->pred_, *current_));
    return *this;
}
```

向前推进：一直往前走直到找到满足谓词的位置（或到达 begin）。使用 do-while 确保至少推进一步。

---

## 进阶任务

- 把 `begin()` 改为用 `std::ranges::find_if` 实现，对比手写 while 循环，确认行为一致。
- 演示 `mutable optional<It>` 版本（把缓存改为 `mutable std::optional<...> cached_begin_`，使 `begin()` 变为 `const`），然后解释它破坏的语义：(a) 两个独立 view 对象共享缓存状态；(b) `mutable optional` 依然需要在拷贝构造中手写清空逻辑——漏掉这一步才是真正的坑。
- 为 `my_filter_view` 迭代器实现完整的 bidirectional 支持，用 `std::ranges::bidirectional_range` concept 验证。
- 把 `non_propagating_cache` 的 `T` 约束为可默认构造，然后观察：如果 iterator 类型不可默认构造，`optional<T>` 的设计为何是正确选择。

---

## 验收点

- `non_propagating_cache` 的拷贝构造和移动构造均不传播缓存值。
- 拷贝 `my_filter_view` 后，新 view 的第一次 `begin()` 会重新扫描底层序列。
- `static_assert(std::copyable<my_filter_view<...>>)` 通过。
- `const my_filter_view` 无法调用 `begin()`（编译期阻止）——这是正确行为，不是 bug。
- 能解释为什么这是"标准 `filter_view` const 迭代受限"的根因。

---

## 观察点

- `non_propagating_cache` 的拷贝清空不是为了性能，而是 view 值语义的守护者：两个独立的 view 对象必须有独立的迭代状态，缓存不能传播。
- `filter_view::begin()` 的非 const 性是有意为之的设计约束，正确表达了"begin() 会修改 view 的可观察状态"这个事实。C++23 不会修复这一点：即使加了 `views::as_const`，`const filter_view` 依然无法迭代。`as_const` 解决的是"元素只读"，不解决"begin() 需要写缓存"。
- 模块 B 里观察到 `filter_view` 缓存 `begin()`：到这里你终于看到了它的实现原理。这是从"使用视角"到"实现视角"的跨越。

---

## 常见坑

- **把 cache 直接写成 `mutable optional<It>`**，忘记在拷贝构造中显式清空，导致两个 view 对象共享缓存状态，违反值语义。
- **non_propagating_cache 的移动构造没有清空缓存**：移动后旧对象应保持"未缓存"状态，否则移动后析构旧对象可能触发悬垂访问。
- **begin() 的实现中没有用缓存**，每次都重新扫描，退化成 O(n) 的重复代价——不会编译失败，但性能悄悄崩塌。
- **忘记 `begin()` 是非 const**：某些场景下（如 `const auto& fv = ...`）调用失败。这是正确的，但不理解原因会误以为是 bug。

---

## 复盘问题

1. 如果 `non_propagating_cache` 的拷贝构造传播了缓存值，会破坏 view 的哪条语义公理？（提示：不是 O(1) copy 性能本身，而是"两个独立 view 的迭代状态应当独立"的值语义）
2. 为什么标准选择把 `begin()` 设为非 const，而不是用 `mutable` 解决？
3. `const filter_view` 的 begin 失败是 view 设计上的"坑"还是正确的语义约束？
4. 如果你在 ranges 管道里把一个含 `filter_view` 的管道存进 `const` 变量，应该怎么处理？
5. `non_propagating_cache` 的"拷贝不传播"与 `std::unique_ptr` 的"拷贝不允许"有什么本质区别？

---

## 对应官方参考

- libstdc++ `<ranges>` 头中 `__non_propagating_cache` 实现（源码路径：`bits/ranges_util.h`）
- cppreference `std::ranges::filter_view` 实现注记
- P0896R4 §Filter view 中关于 begin 缓存的设计说明
- P2278R4（views::as_const 与 basic_const_iterator）
