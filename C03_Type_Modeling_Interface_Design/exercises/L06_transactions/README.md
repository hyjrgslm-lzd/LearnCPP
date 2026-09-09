# L06：异常安全与事务式提交

先读 `../../chapters/06-exception-safety-and-transactions.md`。你只编辑 `src/student/table.hpp`，实现 `l06::Table`。

接口固定：

```cpp
Table(std::initializer_list<int>);
std::span<const TrackedValue> view() const noexcept;
void replace_prefix_basic(std::span<const TrackedValue> source);
void replace_prefix_strong(std::span<const TrackedValue> source);
void replace_all_strong(std::span<const TrackedValue> source);
void swap(Table& other) noexcept;
```

Part 1：`replace_prefix_basic()` 从前往后覆盖。第 k 次复制失败时，已经成功的前缀可以留下，后续旧值不能被破坏，表仍合法。

Part 2：`replace_prefix_strong()` 成功时也只覆盖前缀，但失败时原前缀不变。Reference 做法是复制整张表，在副本上更新前缀，最后 `swap`。另一个合法做法是直接构建最终序列：复制输入前缀，再复制旧表后缀，全部成功后 `swap`。只复制输入前缀再赋值回 live 表不满足 strong guarantee。

Part 3：`replace_all_strong()` 先准备临时 `std::vector<TrackedValue>`，全部复制成功后再 `swap` 提交。checker 覆盖第 1、2、3 次 prepare 复制失败，失败时原表可观察值不变。

Part 4：覆盖尺寸拒绝、成功、空输入、自身 `view()` 输入。自身输入必须先 prepare，不能先清空自身。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-l06 -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-l06 --config Debug
ctest --test-dir build/c03-l06 -C Debug --output-on-failure
```

解析：basic guarantee 允许部分效果，但失败后对象必须仍可析构、可读取、可继续更新。strong prefix 与 basic prefix 的成功语义相同，只加强失败语义；prepare 可以复制整表再改副本，也可以构建最终序列。checker 覆盖 k = 1..5：若该点实际抛出，旧表必须不变；若该点超过实现实际复制次数，操作必须完整成功。strong all 把所有可能失败的复制放在临时对象里；只有 `swap` 是提交点。实验中的第 k 次复制失败来自 checker fixture，只证明受控元素复制失败路径，不证明真实系统内存耗尽。
