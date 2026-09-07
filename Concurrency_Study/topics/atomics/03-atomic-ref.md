# 原子操作 03：给既有对象一个严格的原子访问阶段

假设已有一个普通数据数组，初始化和最终汇总都是串行的，中间某一阶段需要多个线程累加同一个元素。若能直接把声明改成 atomic，通常更清楚；若必须保留普通字段的类型或布局，C++20 的 atomic_ref 提供了一种选择。本篇对应 [E2](../../exercises/E2_atomic_ref/README.md) 与[完整 Reference](../../exercises/E2_atomic_ref/solution.cpp)。

## 1. 引用不拥有数据，也不创建数据

`std::atomic_ref<int> ref(value);` 引用已经存在的 int。它不为 value 提供初值，不复制一份数值当作独立原子，也不延长 value 的寿命。复制 ref 会让两个引用操作同一个对象。多个线程各自构造指向同一对象的 ref，是预期用途。

因此，不能把它理解为“临时加了一把锁”。ref 构造成功并不授予某个线程独占访问权，其他 ref 仍可并发 load、store 或 RMW。只有具体操作的原子性，没有覆盖任意多条语句的临界区。

T 必须满足可平凡复制等接口要求。即使类型可以用 atomic_ref 包装，其原子操作也不一定无锁；实现可以采用共享内部资源。满足对齐是构造前提，不能由一次 is_lock_free 查询来补救已经不合法的构造。

## 2. 三条生命周期线索

正确协议按阶段划分：

1. 构造并普通初始化底层对象，此时没有引用它的 atomic_ref。
2. 构造所需 atomic_ref。只要任何一个仍存在，对该对象的访问都经这些引用进行。
3. 所有并发任务结束，所有 atomic_ref 析构，随后恢复普通访问；最后才销毁底层对象。

第二条不只是“别同时一读一写”。即使同一线程有 ref 存活，也不能随手改用原始对象作普通读写；那会违反 atomic_ref 的独占访问方式要求。ref 本身是 const 也不表示目标只读：`const atomic_ref<int>` 仍可以 store 到非 const int。把“引用包装器不能改”与“底层对象不能改”区分开。

E2 在 worker 函数作用域内构造 ref，在离开函数时销毁；主线程 get 全部 future 后才读取 data[2].value。get 提供完成同步，而且所有引用已离开作用域，所以恢复普通读有明确边界。只等待计数到达某个值，未必能证明所有 ref 的寿命都结束。

如果 ref 指向 vector 元素，并发阶段必须禁止可能移动、销毁元素的操作，包括扩容、erase、clear。预留 capacity 仅解决某些扩容，不等于已经禁止所有失效来源。对同一对象建立整个结构体的 atomic_ref，同时又对其子对象建立另一个 atomic_ref，也违反子对象重叠引用的规则；不要混用粒度。

## 3. 对齐为什么要逐元素考虑

required_alignment 可以比 alignof(T) 更大。给 `int data[4]` 首地址加 alignas，只保证数组开头满足约束；假设 required_alignment 为 8 而 sizeof(int) 为 4，第二个元素地址仍会落在“差 4 字节”的位置。实际机器上的 int 往往没有这个差异，但课程应避免把平台巧合写成普遍保证。

Reference 选择：

```cpp
struct alignas(std::atomic_ref<int>::required_alignment) cell {
    int value = 0;
};
std::array<cell, 4> data{};
```

每个 cell 都必须满足自己的对齐，类型大小也保证数组中的下一个 cell 能继续对齐。value 是唯一的数据成员，位于这个标准布局对象的起始处。程序逐个检查字段地址，之后才构造原子引用。这里底层字段仍是普通 int，但数组元素类型改成了 cell；如果外部 ABI 严格要求连续 int 数组，就需要验证每一个待引用元素的实际对齐或重新设计布局，不能无条件套用此方案。

不要从任意字节缓冲 reinterpret_cast 出一个引用就开始操作。除了地址对齐，还必须已经有合法存活的 T 对象，且访问满足对象模型。打包结构体的字段通常尤其值得检查。

## 4. 运行、观察与解析

在 `Concurrency_Study/exercises` 中：

```powershell
cmake -S E2_atomic_ref -B build/e2 -G "Visual Studio 18 2026" -A x64
cmake --build build/e2 --config Release
ctest --test-dir build/e2 -C Release --output-on-failure
```

四个 worker 各对 data[2].value 做 2000 次 relaxed fetch_add，最终是 8000，邻居仍是 0。其后单独创建 ref 和它的 const 副本，经副本 store 123，再经原 ref 读到 123；两个 ref 都销毁后，普通访问仍读到 123。

relaxed 足够，是因为计算只累计这个元素，不靠计数值发布其他内容；最终的普通读取位于所有任务完成之后。若在并发阶段让某个普通消费者观察同一元素，原有证明就失效。对齐输出只是本机事实；false 的无锁查询不是测试失败。

## 自测与答案

**只有一个 ref，先 ref.store(1)，紧接着 printf 原对象，是否安全？** 这仍违反有 ref 存活时全部访问应经 ref 的要求。应打印 ref.load()，或先结束 ref 作用域再普通读取。

**只给数组首元素对齐，能否保证 data[2]？** 不能普遍保证，取决于元素偏移是否保持 required_alignment 的倍数。逐元素包装可以表达这个需求，但可能改变布局和大小。

**join 后 ref 一定消失吗？** 只有 ref 属于已结束 worker 的局部作用域时才可据此判断。若主线程还持有一个 ref，join 不会替它析构，仍不能恢复普通访问。

**何时优先 atomic 而非 atomic_ref？** 当可以控制字段声明且它始终用于原子访问时，atomic 更直接。atomic_ref 的价值是既有存储和阶段性原子访问，不是绕过声明限制的通用补丁。

## 规范入口

依据 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [atomics.ref.generic.general] 与 [atomics.ref.ops]；设计背景见 [P0019R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0019r8.html)。N5050 已包含 const 目标类型的相应支持与 address() 接口，并按目标类型是否为 const 约束可修改操作。本篇的 C++23 实验不依赖这些较新的接口；“实验没有使用”不能解释为“固定的 C++26 规范没有提供”。
