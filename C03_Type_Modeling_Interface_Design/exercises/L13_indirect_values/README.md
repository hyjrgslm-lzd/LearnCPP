# L13：间接值与多态值语义

先读 `../../chapters/13-indirect-values-and-polymorphic-ownership.md`。本练习是观察型，不提供 Student。程序里的 `clone_value` 是教学类型，只用于说明深复制多态值，不冒充 C++26 `std::indirect` 或 `std::polymorphic`。

Part 1：比较 `unique_ptr`、`shared_ptr` 和 `clone_value`。移动 `unique_ptr` 会转移所有权；复制 `shared_ptr` 会共享对象；复制 `clone_value` 会 clone 动态对象。

Part 2：检查 const 传播。`const clone_value` 只能得到 `const Shape&`，避免通过 const wrapper 修改对象。

Part 3：注入 clone 失败。失败后原对象仍保持原值，说明复制失败不能留下半提交状态。

Part 4：移动 `clone_value` 后源对象进入空状态，访问前必须检查。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L13_indirect_values -B build/author-c-l13 -G "Visual Studio 18 2026" -A x64
cmake --build build/author-c-l13 --config Release
ctest --test-dir build/author-c-l13 -C Release --output-on-failure
```

解析：多态值解决的是“复制包装器时复制动态对象”。它不同于共享身份，也不同于独占句柄。C++26 标准设施的可用性看 `F01_frontier/c26_indirect_polymorphic.cpp`，不是看本教学类型。
