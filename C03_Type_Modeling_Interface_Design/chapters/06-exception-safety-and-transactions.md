# 06：异常安全与事务式提交

异常安全不是“不要抛异常”。异常安全说的是：当操作中途失败，已经存在的对象还满足什么承诺。C02 已讲构造失败、析构收束和资源释放；本章把这些规则提升到接口层，设计可验证的状态提交点。

常见承诺有三种：

- basic guarantee：失败后对象仍合法，可以析构，可以继续使用；部分效果允许留下。
- strong guarantee：失败后对象保持调用前的可观察状态，像操作从未发生。
- no-throw guarantee：操作不抛异常，常用于析构、移动、交换和提交点。

先看批量更新表。`replace_prefix_basic()` 逐个覆盖前缀元素。第 k 次元素复制失败时，前面已经成功覆盖的元素保留，后面未处理的元素保持旧值。这个接口只有 basic guarantee，但它仍然正确：表大小不变，元素都活着，之后还能继续操作。

```cpp
void replace_prefix_basic(std::span<const Item> source) {
    if (source.size() > items_.size()) {
        throw std::out_of_range("prefix too large");
    }
    for (std::size_t i = 0; i < source.size(); ++i) {
        items_[i] = source[i]; // may throw
    }
}
```

如果调用者需要“全部替换或完全不变”，basic 不够。strong guarantee 的常用写法是 prepare/commit：先把所有可能失败的工作放在临时对象里完成，全部成功后用不抛异常的 `swap` 提交。

```cpp
void replace_all_strong(std::span<const Item> source) {
    std::vector<Item> next;
    next.reserve(source.size());
    for (const Item& item : source) {
        next.push_back(item); // may throw
    }
    items_.swap(next);        // no-throw for this allocator setup
}
```

这个设计的提交点是 `swap`。失败注入在 `push_back` 的第 k 次复制发生，旧 `items_` 尚未改动，所以失败后观察值不变。成功路径则一次性替换为新序列。空输入是合法事务，会把表替换为空。自我输入也要先 prepare：`replace_all_strong(table.view())` 不能先清空自身，否则输入视图立即失效。

同一个业务输入也可以只加强失败保证，而不改变成功语义。`replace_prefix_strong()` 和 `replace_prefix_basic()` 都接收前缀输入，成功后都只替换前缀；区别只在失败后状态。basic 版本允许已经完成的前缀留下，strong 版本先复制整张表到临时副本，在副本上更新前缀，最后用不抛异常的 `swap` 提交。不能先复制输入前缀再逐个赋值回 live 表：第二个 live 赋值失败时，第一个元素已经改变，strong guarantee 已破坏。

本章实验用一个受控 `TrackedValue` fixture，在第 k 次复制时抛出 `planned copy failure`。它证明的是教学模型中的元素复制失败路径，不伪装成真实系统内存耗尽或所有分配器行为。真实生产容器还要考虑 allocator propagation、`swap` 的条件 `noexcept`、元素析构是否抛异常等边界。

练习 L06 要实现 `l06::Table`：

```cpp
Table(std::initializer_list<int>);
std::span<const TrackedValue> view() const noexcept;
void replace_prefix_basic(std::span<const TrackedValue> source);
void replace_prefix_strong(std::span<const TrackedValue> source);
void replace_all_strong(std::span<const TrackedValue> source);
void swap(Table& other) noexcept;
```

Part 1：实现 basic 前缀替换。第 2 次复制失败时，checker 期望第 1 个元素已更新，后续元素保持旧值，表仍可继续使用。

Part 2：实现 strong 前缀替换。输入和 Part 1 相同；成功语义相同，失败时原前缀也不变。Reference 使用“整表副本 → 在副本更新前缀 → swap”。另一个合法实现可以直接构建最终序列：先复制输入前缀，再复制旧表后缀，全部成功后 swap。checker 覆盖 k = 1..5；如果该实现实际复制次数较少，超过复制次数的注入点应得到完整成功结果，而不是被当作失败。

Part 3：实现 strong 全量替换。checker 会覆盖 prepare 第 1、2、3 次复制失败；每次失败后原表完全不变。

Part 4：尺寸拒绝、成功、空输入和 self 输入。尺寸拒绝不改变表，且对象之后还能继续成功更新。空输入产生空表；从自己的 `view()` 替换自己仍保持原值。

最小复现命令：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-l06-core -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-l06-core --config Debug
ctest --test-dir build/c03-l06-core -C Debug --output-on-failure
```

若要看 Student 初始失败：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-l06-student -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-l06-student --config Debug
ctest --test-dir build/c03-l06-student -C Debug --output-on-failure
```

预期观察：核心路径中 `reference` 与 `validation_good` 通过，`validation_bad_rejected` 通过，表示坏实现被检查器拒绝；Student 路径中 `L06_transactions_student` 应以 `check failed` 正常失败，不应超时、弹窗或调用 Reference。

完整解析：basic 版本的循环本身就是提交过程，所以失败会留下合法前缀效果。strong prefix 先准备整表副本，在副本上执行同样的前缀更新，只有副本更新成功后才交换。strong all 先准备完整新表，只有复制全部完成才交换；`swap` 不分配，所以可作为提交点。坏变体会先清空再复制，或先复制前缀再用可抛赋值提交到 live 表，checker 应拒绝。
