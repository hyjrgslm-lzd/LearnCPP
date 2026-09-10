# 04 动态数组：原始存储、构造提交和异常回滚

`std::vector` 看起来像“会变长的数组”，但它的实现核心不是数组语法，而是三段状态：

```text
data      -> 一块已分配的原始存储
size      -> 已经构造的元素数量
capacity  -> 原始存储最多能放多少个 T
```

`size <= capacity` 是基本不变量。`[data, data + size)` 里有活着的 `T` 对象；`[data + size, data + capacity)` 只是适合放 `T` 的原始存储，还没有对象。你可以在未构造位置上构造对象，不能把它当已经存在的 `T` 来读写。

本章实现一个教学版 `dynamic_array<T>`。它只覆盖增长、尾插、尾删、清空、移动和借用观察，不承诺 `std::vector` 兼容。没有 allocator 模板参数，没有 insert/erase，没有自定义 iterator，没有 shrink，没有 pmr，也不讲 C07 的内存池。

## 类型边界

为了让异常安全边界清楚，教学版要求：

- `T` 不是 `const` 或 `volatile`。
- `T` 析构是 `noexcept`。
- `T` 可复制，或 `T` 的移动构造是 `noexcept`。

这个边界来自增长时的强保证。扩容需要把旧元素搬到新存储。如果移动可能抛异常，又没有复制兜底，搬到一半失败时旧数组可能已经有一部分元素被移动走，原状态无法可靠恢复。标准 `vector` 有更复杂的规则和 allocator 交互；教学版选择更小的可解释接口。

## 原始存储不等于对象

分配器 `allocate(n)` 只给你能放 `n` 个 `T` 的存储。真正建立对象要调用 `std::allocator_traits<Alloc>::construct` 或等价构造操作。销毁对象要调用 `destroy`。释放存储要调用 `deallocate`。

顺序不能乱：

```text
allocate raw storage
construct elements one by one
use elements
destroy constructed elements
deallocate raw storage
```

如果构造第 k 个元素时抛异常，只能销毁已经构造成功的前 k 个，不能销毁未构造的位置。这就是“构造提交”的基础：`size_` 只有在对象成功构造后才能增加。

## 不扩容尾插

容量足够时，`push_back(T value)` 只在尾部未构造位置建立一个对象：

```cpp
construct(data_ + size_, value_or_move);
++size_;
```

`++size_` 必须在构造成功之后。如果先增加 `size_`，构造抛异常时析构函数会以为尾部对象已经存在，后续 `clear()` 可能销毁一个从未构造的地址。Student 初态应该安全失败，bad 变体会专门模拟“提前提交状态”的错误。

## 扩容尾插：先新尾，再搬旧元素

扩容时要申请更大存储。为了让“新元素构造失败”不影响旧数组，可以先在新存储的尾部构造新元素，再搬旧元素：

```text
old: [A B] capacity=2
push C, requested=4

new raw: [ _ _ _ _ ]
construct C at new[2]
relocate A to new[0]
relocate B to new[1]
commit: destroy old A/B, deallocate old, data=new, size=3, capacity=4
```

如果构造 C 失败，旧数组完全没动。若搬旧元素到一半失败，销毁新存储里已经构造的前缀和新尾，释放新存储，旧数组仍保持原状态。

这也是为什么类型边界要求“可复制或 noexcept move”。如果移动不会抛，可以移动旧元素；否则用复制保留旧元素。复制失败时，旧元素还在原地。

## `reserve`：只增长容量，不改变 size

`reserve(n)` 的契约是容量至少为 n，元素个数不变。`n <= capacity()` 时什么都不做。`n > max_size()` 时抛 `std::length_error`，不能尝试分配一个溢出的大小。

增长成功之前，旧状态不能改变。正确顺序是：

1. 分配新存储。
2. 逐个复制或 noexcept 移动旧元素到新存储。
3. 如果失败，销毁新存储里已构造对象，释放新存储，旧数组不变。
4. 全部成功后，销毁旧元素，释放旧存储，提交 `data_` 和 `capacity_`。

`reserve` 不构造空位里的对象。`capacity` 表示存储能力，不表示有这么多个 `T` 活着。

## 借用失效

`dynamic_array<T>::view()` 返回 `std::span<T>` 或 `std::span<const T>`。这是借用视图。它不拥有元素，也不延长数组生命期。

如果后续 `reserve` 或 `push_back` 触发重分配，旧 span 的地址就过期。练习 checker 会保存旧 `data()`，触发扩容后比较新旧地址；不会解引用旧 span。未定义行为不能作为稳定验收证据。

移动构造和移动赋值采用“转移整块存储”的策略。旧借用地址会跟随存储到新对象，但从接口设计角度，调用者不应继续通过已移动源对象理解那段借用。练习只验证目标对象接管数据、源对象变空。

## 反例：提交太早

一个真实错误是扩容时先把 `data_` 和 `capacity_` 指向新存储，再构造新尾：

```cpp
data_ = next;
capacity_ = next_capacity;
construct(data_ + old_size, value); // 这里可能抛
```

一旦构造抛异常，对象已经丢失旧 `data_`，状态也声称自己有新容量。轻则泄漏旧元素，重则析构时销毁错误存储。正确做法是把新存储交给局部清理对象管理，直到所有构造都成功，最后一步才提交成员变量。

另一个错误是空 `pop_back()` 静默返回。这会隐藏调用者的边界 bug。教学版选择抛 `std::out_of_range`，checker 明确覆盖。

## 实验

`L05_dynamic_array` 是实现型练习：

- `src/student/dynamic_array.hpp` 是安全、有限、未完成的 Student 初态，构建可过，运行会失败。
- `src/reference/dynamic_array.hpp` 是完整参考实现。
- `validation/good/dynamic_array.hpp` 用 `std::vector<T>` 独立模拟公开接口，验证 checker 不只认参考代码。
- `validation/bad/dynamic_array.hpp` 保留提前提交容量的真实错误，bad 测试必须因预期诊断退出 1。
- `main.cpp` 使用 `<check.hpp>`，真实消费当前 include 路径里的 `dynamic_array.hpp`。

检查覆盖普通 int、move-only 元素、移动会抛但可复制元素、over-aligned 元素、空 pop、超大 reserve、扩容前后借用地址和异常回滚。

## 解析

动态数组的难点不在“指针加一”，而在失败路径。每一步都要知道哪些对象已经活着、哪些只是存储、谁负责销毁、何时可以改变公开状态。只要成员状态提前提交，异常就会把对象留在谎报状态；只要 `size_` 提前增加，析构就可能处理不存在的对象。

标准 `vector` 还要处理 allocator 传播、插入位置、范围构造、迭代器类型和更多异常规则。本章不伪装覆盖这些内容。它只把连续存储容器最核心的提交/回滚讲清楚，为后续 `vector`、`inplace_vector`、`span` 借用和源码阅读建立最低可靠模型。
