# L03：optional 空状态观察

先读 `../../chapters/03-optional-and-empty-state.md`。本练习是观察型程序，不要求编辑 Student 文件。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L03_optional -B build/c03-l03 -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-l03 --config Debug
ctest --test-dir build/c03-l03 -C Debug --output-on-failure
```

Part 1：观察 `optional<T>` 何时真的拥有 `T`。`emplace()` 开始对象生命期，`reset()` 结束生命期。

Part 2：观察 `emplace()` 构造失败。旧值先被销毁，新值构造抛异常后，`optional` 为空。

Part 3：观察访问前提。`value()` 在空状态抛 `std::bad_optional_access`；`*` 和 `->` 只能在已经证明有值后使用。

Part 4：观察 `value_or()`。默认实参会先求值；对右值 `optional` 调用时，有值对象可被移动。

Part 5：观察 C++23 monadic 操作。`and_then` 返回 `optional`，`transform` 包装返回值，`or_else` 只在空状态运行；回调抛异常会传播。

解析：`optional` 只适合表达正常缺失。它拥有内部对象，所以可安全返回值快照；C++23 没有 `std::optional<T&>`。C++26 的引用 optional 与 0/1 range 由 F01 探测，不用本练习伪造支持。
