# 练习 L02：值类别、绑定与 forward

先读 [02. 表达式与引用](../../chapters/02-expressions-and-references.md)。本题是观察型练习，不提供 Student。

| Part | 操作 | 验证 |
|---|---|---|
| L02-1 | 用 `decltype` 观察 lvalue/xvalue/prvalue | `decltype((item))` 是 lvalue reference，`decltype(as_xvalue(...))` 是 rvalue reference |
| L02-2 | 观察引用绑定 | 非 const lvalue reference 不能绑定 rvalue，rvalue reference 可绑定 xvalue |
| L02-3 | 比较 `auto` 与 `decltype(auto)` | `auto` 丢引用，`decltype(auto)` 保留引用 |
| L02-4 | 观察 forwarding reference | lvalue 参数使 `T` 推导为引用，prvalue 不会 |
| L02-5 | 编译 negative | 错误绑定和错误 forward 都必须拒绝 |

运行：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L02_value_categories -B C02_Objects_Lifetime_Ownership/exercises/L02_value_categories/build/c02-foundation-author -G "Visual Studio 18 2026" -A x64
cmake --build C02_Objects_Lifetime_Ownership/exercises/L02_value_categories/build/c02-foundation-author --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L02_value_categories/build/c02-foundation-author -C Debug --output-on-failure
```

解析：`std::move` 只把表达式变成 xvalue；有名字的 `value` 在函数体里仍是 lvalue，所以错误 forward 版本无法传给只接受 `int&&` 的函数。`decltype(auto)` 保留引用，能修改原对象；普通 `auto` 复制值。
