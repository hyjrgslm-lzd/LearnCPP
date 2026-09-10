# A01：值级 constexpr 表

先读[值级编译期计算正文](../../chapters/18-compiletime-values.md)。本题固定一个字面量 key/value 表：key 最长 15 个 ASCII 字符，value 是 `int`。`row::key_width == 16` 包含终止 NUL；空 key `""` 允许，payload 内部 NUL 和非 ASCII byte 拒绝。学生只编辑 `src/student/compiletime_values.hpp`。

实现目标：

- `make_row("hp", 100)` 把字符串字面量变成可作为 NTTP 输入的固定宽度 row。
- `make_row("123456789012345", 15)` 合法；16 个字符、非 ASCII 和 `"a\0b"` 这类内部 NUL 必须编译失败。
- `sort_by_key` 在编译期稳定排序，重复 key 的相对顺序不变。
- `make_table<raw>()` 先按原始顺序去重，重复 key 保留第一项，再排序；返回 `table<N>`，其中 `N` 是去重后的大小。
- `find(table, "hp")` 对已排序表做二分查找，返回 `std::optional<int>`。
- `normalize_same_size(raw)` 展示普通 consteval 参数能算值，但返回类型仍是输入大小；改变结果类型形状需要把 raw 放进模板参数。

检查器会拒绝“只线性查找未规范化表”“重复 key 保留最后值”“没有稳定排序”“空输入不返回 `table<0>`”“把 embedded NUL 查询误判为空 key”这类错误。`validation/bad` 的错误是保留最后重复值，不是固定返回失败标记。`validation/diagnostics` 额外覆盖 16 字符、非 ASCII、内部 NUL 和 scratch pointer 逃逸。

本机命令：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/A01_compiletime_values -B C04_Generic_CompileTime_Reflection/exercises/A01_compiletime_values/build/local -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/A01_compiletime_values/build/local --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/A01_compiletime_values/build/local -C Debug --output-on-failure
```
