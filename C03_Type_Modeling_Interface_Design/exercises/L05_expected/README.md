# L05：expected 错误通道观察

先读 `../../chapters/05-expected-and-error-channels.md`。本练习是观察型程序，不要求编辑 Student 文件。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L05_expected -B build/c03-l05 -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-l05 --config Debug
ctest --test-dir build/c03-l05 -C Debug --output-on-failure
```

Part 1：观察 `expected<T,E>` 的成功值与 `unexpected<E>` 错误值。

Part 2：观察 `expected<void,E>`。成功没有值，但失败仍保留错误。

Part 3：观察 observer 分类。`*`、`->` 和 `error()` 有状态前提；`value()` 在错误状态按定义抛 `std::bad_expected_access<E>`。

Part 4：观察 `value_or()` 与 `error_or()`。默认实参会先求值；不要用它们隐藏必须处理的错误。

Part 5：观察 monadic 操作。成功通道用 `and_then/transform`，错误通道用 `or_else/transform_error`；回调抛异常会传播。

Part 6：观察 move-only 成功值。`expected<std::unique_ptr<T>, E>` 可移动不可复制，链式消费要显式移动。

Part 7：观察 C++23 借用返回。`expected<T&, E>` 不合法；用 `expected<reference_wrapper<T>, E>` 表达可失败借用，并明确它不延长对象生命期。

解析：`expected` 负责传递可处理错误，不负责自动回滚对象状态。业务拒绝进入错误类型；分配失败、程序错误和跨线程异常要按层级单独设计。
