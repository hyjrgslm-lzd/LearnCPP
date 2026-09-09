# 练习 L01：初始化观察与编译反例

先读 [01. 初始化](../../chapters/01-initialization.md)。本题是观察型练习，不提供 Student。目标是把可运行观察和必须拒绝的编译反例分开。

| Part | 操作 | 验证 |
|---|---|---|
| L01-1 | 观察 default/value initialization | 程序只读取 value-initialized 标量，不读取未初始化标量 |
| L01-2 | 观察 list/aggregate/designated initialization | aggregate 成员按声明顺序得到值 |
| L01-3 | 观察 static/dynamic initialization 与 `constinit` | `constinit` 常量初始化通过，动态初始化由函数调用完成 |
| L01-4 | 观察 brace initializer 求值顺序 | `Pair{mark(1), mark(2)}` 固定左到右 |
| L01-5 | 编译 negative | `narrowing` 和 `constinit_dynamic` 必须拒绝 |

运行：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L01_initialization -B C02_Objects_Lifetime_Ownership/exercises/L01_initialization/build/c02-foundation-author -G "Visual Studio 18 2026" -A x64
cmake --build C02_Objects_Lifetime_Ownership/exercises/L01_initialization/build/c02-foundation-author --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L01_initialization/build/c02-foundation-author -C Debug --output-on-failure
```

解析：default-initialized 自动标量存在，但值未确定，本题不读取它。`int{}`、`Defaults{}` 和 `std::array{...}` 都走安全检查。窄化和 `constinit` 动态初始化是编译期反例；通过 CTest wrapper 匹配诊断文本，而不是运行错误程序。
