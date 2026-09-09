> 对应章节：模块 H — 高级实现模式 / H-3 generator promise_type + basic_const_iterator（P2502R2, P2278R4）

## 目标

读懂 `std::generator` 的 promise_type 核心骨架，理解 `initial_suspend` / `final_suspend` / `yield_value` / `elements_of` 各自为何如此设计；并实现 `my_basic_const_iterator<I>` 的最小骨架，理解它与"在 iterator 上加 const"的根本区别。

---

## 前置理解

- **协程基础**：你理解 `promise_type`、`coroutine_handle`、`co_yield`、`co_return`、`initial_suspend`、`final_suspend`。
- **generator 是 input_range**：协程状态是唯一的，同一个 generator 不能有两个独立的迭代器同时遍历，因此只满足 `input_range`，不满足 `forward_range`。
- **basic_const_iterator 的正确理解**：`const std::vector<int>::iterator it` 只让变量 `it` 不可修改，但 `*it` 仍然是 `int&`（可写）。`my_basic_const_iterator<I>` 通过重写 `operator*` 返回 `const value_type&`，才真正实现"解引用只读"。
- **P2502R2（std::generator）**：协程与 ranges 桥接的标准方案，C++23 引入。它满足 `input_range + view`，是 move-only（协程句柄唯一所有权）。
- **P2278R4（basic_const_iterator）**：C++23 引入。在 C++20 里没有这个工具，要实现"只读视图"只能用 `const T&` 约束或手写只读迭代器。

---

## 必做任务

### 任务 1 — 实现 initial_suspend（返回 suspend_always）

```cpp
std::suspend_always initial_suspend() noexcept { return {}; }
```

**为什么是 suspend_always**：惰性启动。generator 被构造时，协程体尚未开始执行。只有调用方第一次调用 `begin()`（即第一次 resume）时，协程才开始运行直到第一个 `co_yield`。若改为 `suspend_never`，协程在构造时立刻运行，调用方无法安装迭代器。

### 任务 2 — 实现 final_suspend（返回 suspend_always）

```cpp
std::suspend_always final_suspend() noexcept { return {}; }
```

**为什么是 suspend_always**：保留 done 状态。协程执行完毕后，`handle.done() == true` 可被稳定读取，迭代器 `operator==` 检测此状态。若改为 `suspend_never`，协程帧自动销毁：之后检查 `handle.done()` 是 UB；析构 generator 时再 `handle.destroy()` 是 double-free。

### 任务 3 — 实现 yield_value

```cpp
template<class T>
    requires std::convertible_to<T, Val>
std::suspend_always yield_value(T&& val) noexcept {
    current_value_ = std::addressof(val);
    return {};
}
```

**为什么存地址而非拷贝**：调用方在协程挂起后通过指针读取，值在 `co_yield` 表达式的生命期内有效，不需要拷贝。

**为什么返回 suspend_always**：确保值被消费前协程不继续执行。若返回 `suspend_never`，值还没被读取协程就继续运行到下一个 `co_yield` 或结束，`current_value_` 指向已销毁的临时量——悬垂指针，没有编译错误，只有运行时 UB。

### 任务 4 — 实现 iterator::operator++()

```cpp
iterator& operator++() {
    handle_.resume();
    if (handle_.promise().exception_)
        std::rethrow_exception(handle_.promise().exception_);
    return *this;
}
```

resume 协程推进到下一个 `co_yield` 或协程结束，异常通过 `exception_` 传播。

### 任务 5 — 实现 iterator::operator*()

```cpp
const Val& operator*() const noexcept {
    return *handle_.promise().current_value_;
}
```

通过 promise 的 `current_value_` 指针读取当前 yield 的值。

### 任务 6 — 实现 iterator::operator==(default_sentinel_t)

```cpp
bool operator==(std::default_sentinel_t) const noexcept {
    return handle_.done();
}
```

`handle_.done()` 返回 true 当且仅当协程已到达 `final_suspend`（即 `final_suspend = suspend_always` 时）。

### 任务 7 — 实现 my_basic_const_iterator::operator*()

```cpp
constexpr const value_type& operator*() const
    noexcept(noexcept(static_cast<const value_type&>(*current_)))
{
    return static_cast<const value_type&>(*current_);
}
```

把 `*current_`（可能是 `T&`）cast 为 `const T&`，产出只读引用。

### 任务 8 — 实现 operator++() 和条件 operator--()

```cpp
constexpr my_basic_const_iterator& operator++() {
    ++current_; return *this;
}
constexpr my_basic_const_iterator& operator--()
    requires std::bidirectional_iterator<I>
{ --current_; return *this; }
```

const 化只影响解引用的可变性，不影响迭代器的推进能力——iterator_concept 继承底层，随机访问能力保留。

---

## 进阶任务

- 为 `my_generator` 添加异常传播：在 `iterator::operator++` 里检查 `promise().exception_`，若非空则 rethrow，验证 `co_yield` 之后协程体抛出的异常能传播到调用方。
- 实现最小语义的 `elements_of`（不含 symmetric transfer）：在 `yield_value` 重载里接受一个 range，for 循环 yield 每个元素，理解语义后再理解 symmetric transfer 为什么更优。
- 扩展 `my_basic_const_iterator` 以支持完整的 C++23 `basic_const_iterator` 要求：包括 `iter_const_reference_t`、`common_reference_t` 的正确推导，以及与 `views::as_const` 的配合。
- 测量模板膨胀：写一个五层 view 管道，用 `nm` 或 `dumpbin /SYMBOLS` 统计编译后目标文件中因该管道产生的符号数量，对比 `ranges::to<vector>` 物化后的符号数量。

---

## 验收点

- `my_generator<int>` 能产出 fibonacci 数列，通过 `views::take(8)` 正确停止，迭代器不 double-resume。
- `initial_suspend = suspend_always` 和 `final_suspend = suspend_always` 的原因能用一句话说清楚，不与 `suspend_never` 混淆。
- `yield_value` 存的是值的地址（不拷贝），且返回 `suspend_always`——能解释为什么这两个选择缺一不可。
- `my_basic_const_iterator<I>` 的 `operator*` 返回 `const value_type&`，`static_assert` 验证类型正确。
- 能用一句话区分"basic_const_iterator<I>"和"const I"各自意味着什么。

---

## 观察点

- `std::generator` 是 C++23 里把协程结果接入 ranges 管道的标准桥梁（P2502R2）。它本身满足 `input_range + view`，是 move-only——不同于 `filter_view` 的可拷贝性，根本原因是协程句柄的唯一所有权 vs view 的借用语义。
- `final_suspend = suspend_always` 使 generator 的"协程已结束"状态可被稳定读取，这与 `iterator::operator==` 的实现直接相关——operator== 检查的就是 `handle.done()`。
- `basic_const_iterator` 不是 C++20 就有的：它是 P2278R4 在 C++23 引入的。在 C++20 里，要实现"只读视图"，只能用 `const T&` 约束或手写只读迭代器。
- **模板膨胀**：每加一层 view adaptor 产生一个新模板实例化。五层管道 = 五个不同类型，在 debug 构建下，每个模板实例会产生独立的调试信息，链接时 debug info 体积可能比 release 二进制本身大 10 倍以上。应对策略：及早 `ranges::to<vector>` 物化、用 `any_view` 类型擦除（range-v3，非标准）、不把管道类型作为公开函数签名的参数/返回类型。

---

## 常见坑

- **把 `final_suspend` 写成 `suspend_never`**：协程帧自动销毁，iterator 再检查 `handle.done()` 是 UB；析构 generator 时再 `handle.destroy()` 是 double-free。没有编译错误，只有运行时崩溃。
- **`yield_value` 返回 `suspend_never`**：值还没被消费，协程继续执行，`current_value_` 悬垂——运行时 UB，没有编译错误。
- **把 `basic_const_iterator<I>` 当作 `const I` 的别名**：`const std::vector<int>::iterator` 的 `operator*` 仍然返回 `int&`（可写），与 `basic_const_iterator` 的 `const int&` 完全不同。
- **elements_of 语义误解**：`co_yield sub_gen` 直接 yield 一个 generator 对象本身，不会递归产出元素。`elements_of` 才是"把嵌套 generator 的每个值逐一 yield 给外层"的正确形式。

---

## 复盘问题

1. 如果 `initial_suspend` 返回 `suspend_never`，generator 的构造行为会发生什么变化？
2. `yield_value` 为什么存地址而不存值的拷贝？
3. `std::generator` 是 move-only 的，这与 `filter_view` 的可拷贝性形成对比——根本原因是什么？
4. `basic_const_iterator<I>` 的 `iterator_concept` 应该等于 I 的 `iterator_concept`，还是降级？
5. 模板膨胀在什么场合会真正成为工程问题，而在什么场合可以忽略？

---

## 对应官方参考

- P2502R2 `std::generator` 完整提案（promise_type 骨架、elements_of 语义）
- P2278R4 `views::as_const` 与 `basic_const_iterator` 提案
- libstdc++ `<generator>` 头实现（`include/std/generator`）
- cppreference: `std::generator` / `std::basic_const_iterator`
