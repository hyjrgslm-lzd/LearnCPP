# 15 `object_buffer<T>`：受限资源容器

本章把 11-14 章合成一个小容器：`object_buffer<T>`。它只承担 C02 需要的责任：拥有一段连续存储，手动管理其中 `[0, size_)` 的对象生命期，提供只读 view，并在增长和插入失败时保持强保证。

它不是 `std::vector` 替代品。本课故意不做 iterator、mutable view、复制容器、自定义 allocator、pmr、erase、insert range、small-buffer optimization。范围越小，越容易看清存储、生命期、别名和异常保证的交点。

```cpp
object_buffer<T> b;
b.reserve(8);
b.push_back(T{});
b.pop_back();
b.clear();
std::span<const T> view = b.view();
```

## 数据成员和核心不变量

实现只需要三类状态：

```text
data_      allocator<T> 返回的起始指针，空容器可为 nullptr
size_      已经构造完成的 T 对象个数
capacity_ 这段 allocation 可容纳的 T 槽数
```

`std::allocator<T>::allocate(capacity_)` 给出足够对齐的存储，并按标准模型建立 `T[capacity_]` 数组对象边界；它不构造元素。只有 `[data_, data_ + size_)` 中的元素生命期已经开始。`[data_ + size_, data_ + capacity_)` 是可用槽，不是可读取对象。

这导出几个硬不变量：

- `size_ <= capacity_`。
- `capacity_ == 0` 时 `data_ == nullptr`。
- 析构只销毁 `[0, size_)`，不能碰未构造槽。
- `view()` 返回 `std::span<const T>{data_, size_}`。
- 任何抛异常路径都不能让外部观察到半提交状态。

## 类型前提：trait 与语义分开

课程实现约束 `T`：完整非 cv 对象、nothrow destructible，并且 copy constructible 或 nothrow move constructible。

```cpp
static_assert(std::is_nothrow_destructible_v<T>);
static_assert(std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>);
```

这些 trait 只能证明语法和异常规格。它们不能证明复制没有外部副作用，不能证明移动后源对象仍满足业务期望，也不能证明析构日志不会分配。checker 用受控 fixture 检查本容器的提交顺序；通用 `T` 的业务语义仍由调用者负责。

throwing-move-only 类型被拒绝。原因很具体：增长时要把旧元素转移到新存储。如果 move 可能抛，且类型不可 copy，失败后旧元素可能已经被部分移动，无法提供强保证。

## `reserve(n)` 的强保证

`reserve(n)` 的目标是确保 `capacity() >= n`。当 `n <= capacity_` 时，它必须无操作：不分配、不移动、不改变借用。

增长时顺序如下：

1. 检查 `n` 是否超过 allocator 最大元素数，增长计算是否溢出。失败直接抛 `std::length_error`，状态不变。
2. 分配新存储到局部变量 `new_data`。还不写 `data_` 或 `capacity_`。
3. 把旧元素构造到新存储。若 `T` 的 move 构造是 `noexcept`，使用 move；否则使用 copy fallback。
4. 若第 k 个旧元素构造失败，只销毁新存储中已经成功构造的 `[0, k)`，释放 `new_data`，旧容器完全不动。
5. 全部成功后，销毁旧 `[0, size_)`，释放旧 allocation。
6. 最后提交 `data_ = new_data; capacity_ = n;`，`size_` 保持原值。

错误实现通常先提交容量：

```cpp
capacity_ = n;                 // 错：外部状态已经改变
auto* new_data = alloc.allocate(n);
// 后续 copy 第 k 个元素失败，容器无法恢复旧 capacity 观察值
```

强保证不只要求内存不泄漏，还要求调用失败后 `size()`、`capacity()`、`view()` 内容和旧借用状态都与调用前一致。

## `push_back(T value)`：按值参数的意义

接口选择 `push_back(T value)`，而不是 `const T&` / `T&&` 重载，是为了把“构造参数对象”和“插入容器”分开。函数体开始前，`value` 已经是一个独立对象。

这让自身 view 元素追加可定义：

```cpp
object_buffer<std::string> b;
b.push_back(std::string{"alpha"});
b.push_back(b.view()[0]); // 参数 value 先复制完成；扩容后旧 view 失效也不影响 value
```

但也要写清边界：构造 `value` 时发生的异常或外部副作用不属于 `push_back` 的回滚范围。强保证只覆盖函数体内对容器自身状态的改变。

## 无扩容插入

当 `size_ < capacity_` 时，尾槽已有可用存储但还没有活跃对象。正确顺序是：

```cpp
std::construct_at(data_ + size_, std::move(value));
++size_;
```

如果构造尾元素抛异常，`size_` 尚未增加，尾槽没有活跃对象；析构不能碰这个槽。错误顺序是先 `++size_` 再构造，因为失败路径会让容器认为未构造槽已经活跃。

## 扩容插入：先新尾，再迁移旧元素

`push_back` 触发扩容时，`value` 已经存在，但把它构造到新存储的动作仍可能抛异常。若先迁移旧元素，再构造新尾，新尾失败时你已经对旧元素做过 move/copy，清理和回滚更复杂，甚至破坏强保证。

冻结顺序采用：

1. 计算新容量，检查长度。
2. 分配 `new_data`。
3. 在 `new_data + size_` 构造新尾元素。
4. 若新尾构造失败，只释放 `new_data`；不要销毁 `new_data + size_`，因为对象没有构造成功；旧容器不动。
5. 新尾成功后，迁移旧 `[0, size_)` 到 `new_data[0..size_)`。
6. 若 copy fallback 第 k 个旧元素失败，销毁新前缀 `[0, k)` 和已经成功的新尾，释放 `new_data`；旧容器不动。
7. 迁移全部成功后，销毁旧元素，释放旧存储，提交 `data_`、`capacity_`，最后 `++size_` 或提交新 size。

这里的清理集合要精确。新尾失败时没有新尾对象，不能 destroy。旧元素迁移失败时，新尾已经成功，必须 destroy；新前缀只 destroy 成功构造的数量，不能按目标 `size_` 粗暴销毁。

## 回滚实现：显式 catch 与 RAII

强保证可以用显式 `try`/`catch (...)` 写：

```cpp
T* next = alloc.allocate(requested);
std::size_t built = 0;
try {
    for (; built != size_; ++built) {
        construct_relocated(next + built, data_[built]);
    }
} catch (...) {
    destroy_prefix(next, built);
    alloc.deallocate(next, requested);
    throw;
}
```

这段 C++ 语义是合法的：catch 负责清理局部半成品，`throw;` 重新抛出原异常，调用者仍看到自然传播的构造失败。问题出在本课实测工具链的一个窄路径：Clang 22.1.3 + Windows ASan + MSVC exception handling 组合下，独立最小 `rethrow_only` 也会崩溃；`throw_only` 通过，无 ASan 通过。因此不能从 P1 ASan 崩溃反推“所有 `throw;` 都错”或“P1 强保证算法错”。

为了让课程验证避开这个本机插桩限制，同时保留同一 C++ 语义，Reference 改成 RAII rollback：

```cpp
T* next = alloc.allocate(requested);
std::size_t built = 0;

auto rollback = [&](T* ptr) noexcept {
    destroy_prefix(ptr, built);
    alloc.deallocate(ptr, requested);
};
std::unique_ptr<T, decltype(rollback)> cleanup(next, rollback);

for (; built != size_; ++built) {
    construct_relocated(next + built, data_[built]);
}
cleanup.release();
```

异常仍自然离开函数，但清理由 `unique_ptr` 的 noexcept deleter 在栈展开时完成。成功路径调用 `release()`，之后才销毁旧元素、释放旧 storage、提交新指针和容量。这个改法不是禁用 ASan，也不是删除失败注入；它只把手写 catch/rethrow 变成等价的作用域回滚。

## 最小失败注入输入

checker 不靠学生回填报告，它用受信 fixture 持有计数器、事件日志和失败点。最小输入覆盖这些错误：

| 场景 | 初始状态 | 失败点 | 期望 |
| --- | --- | --- | --- |
| 无增长 reserve | `size=2, capacity=4` | 无 | 地址、容量、元素、借用不变 |
| 超限/溢出 | 任意 | `reserve(max+1)` | 抛 `length_error`，状态不变 |
| copy 第 k 次失败 | 旧元素 3 个，copy fallback | copy 第 2 个旧元素抛 | 新前缀 1 个被销毁，旧 3 个仍活跃 |
| 新尾失败 | 满容量，`push_back(value)` | 构造新尾抛 | 只释放新分配，旧状态不变，不 destroy 未构造尾 |
| 迁移失败 | 新尾成功，copy 旧元素第 k 个失败 | copy 抛 | destroy 新尾和成功前缀，旧状态不变 |
| move-only noexcept | move-only 且 move 不抛 | 无 | 增长成功，旧元素被合法移动后销毁 |
| throwing-move-only | 不可 copy，move 可能抛 | 编译期 | 拒绝实例化 |
| overalligned/nontrivial | `alignas` + 析构日志 | 无 | 对齐正确，析构次数精确 |
| self move | `b = std::move(b)` | 无 | 状态无损，不 double destroy |

这些输入直接对应实现顺序。若一个实现先提交 `capacity_`、先增加 `size_`、失败时按目标数量销毁，至少一个场景会失败。

## 借用契约

`view()` 是 `std::span<const T>`，不是 owner。它的元素指针只在对应元素生命期和底层 allocation 保持期间有效。

操作对借用的影响：

| 操作 | 旧元素借用 | 旧 span 长度 | 说明 |
| --- | --- | --- | --- |
| `reserve(n <= capacity)` | 保持 | 保持 | 无操作 |
| `push_back` 不扩容 | 旧元素保持 | 旧 span 长度不变 | 新元素不出现在旧 span 中 |
| `push_back` 扩容 | 全部失效 | 全部失效 | 旧 allocation 被释放 |
| `pop_back` | 被弹元素失效 | 旧 span 若覆盖尾部则不能再用 | 其他元素保持 |
| `clear` | 全部元素失效 | 全部失效 | capacity 可保留 |
| move construction | 借用随 allocation 转到新 owner | 指向元素仍由新 owner 维持 | 源对象变空 |
| move assignment | 目标旧借用失效，源借用转到目标 | 取决于原 owner | 目标旧数据先销毁 |

只读 span 是本课 API 的刻意选择。若给出 mutable span，调用者可以在容器维护之外修改元素，甚至在异常测试中插入额外副作用，checker 范围会被扩大。iterator 和完整 vector 语义也会引入更多失效规则，不属于 C02。

## 与前几章的对应关系

- 第 11 章：`data_ + size_` 是活跃元素范围的 one-past；`data_ + capacity_` 是 allocation 数组边界的 one-past。二者不能混用。
- 第 12 章：`allocator<T>::allocate` 不构造元素；每个元素由 `construct_at` 开始生命期，由 `destroy_at` 结束生命期。
- 第 13 章：不能用 `reinterpret_cast<T*>` 让未构造槽变成对象；view 的旧指针在扩容后不能靠地址复用复活。
- 第 14 章：所有检查和状态提交必须排在 UB 风险之前；失败路径证据要证明程序真的启动并检查了受信状态。

## 下游回访

Coroutine：协程 frame 或闭包保存 `span` 时，span 不保活 `object_buffer`。如果协程挂起后 owner 扩容、clear、析构或 move assignment，恢复时使用旧 span 就是生命期问题。

Ranges：ranges 管道可能延迟执行。`auto r = b.view() | views::filter(...);` 不拥有元素；执行 `r` 前必须保证 `b` 仍存在且没有让相关元素失效。

Execution：operation state、receiver 和 scheduler 的归属必须明确。把 receiver 中的 `span` 当成资源所有权，会在异步完成时读到已结束生命期的对象。

Concurrency：`object_buffer<T>` 是单线程容器。锁、ABA、hazard pointer 这些主题不改变本课核心事实：对象生命期结束后，旧指针不能因地址数值复用而重新有效。

## 本章实验怎么读

P1 checker 直接观察真实被测对象和受信 fixture。good 实现必须通过；bad noop、早提交、漏析构、失败清理错误等实现必须被拒绝。Student 不能回填完成标记或报告；资源和事件状态由 checker 持有。

当前 P1 Reference 采用 RAII rollback 版本。旧 ASan wrapper 失败 JSON 保留为命令启动失败证据；实际 runtime 结论以匹配运行时 `PATH` 的直接 exe 记录为准。
