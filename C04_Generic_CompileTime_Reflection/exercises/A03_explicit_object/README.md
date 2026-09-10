# A03：deducing this 与 `forward_like`

先读[显式对象参数正文](../../chapters/20-explicit-object-forwarding.md)。学生只编辑 `src/student/explicit_object.hpp`。

本题从四个 `value()` 重载开始：`&`、`const&`、`&&`、`const&&` 都必须保留成员对象的 cv/ref 类别、地址和 `noexcept`。然后用 C++23 显式对象参数把四个重载合成一个：

```cpp
constexpr decltype(auto) value(this auto&& self) noexcept;
```

`forward_like<decltype(self)>(self.member)` 把对象的类别投射到成员上：左值对象得到左值引用，右值对象得到右值引用，const 对象得到 const 引用。`take()` 只允许 mutable rvalue，防止从 `const&&` 假装移动 move-only 对象。

检查器覆盖：

- 四类 cv/ref 返回类型、地址和值写回。
- `noexcept` 是否保留。
- `slot<std::unique_ptr<int>>` 只能从 mutable rvalue move。
- explicit-object recursive lambda 遍历链表。
- `name_holder::name()` 返回外部借用；它不延长临时对象生命周期，调用方不能保存来自临时对象的引用。

`validation/bad` 返回副本并只计算链表头，检查器必须拒绝。`validation/diagnostics` 证明 `const&& take()` 被删除。

本机命令：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/A03_explicit_object -B C04_Generic_CompileTime_Reflection/exercises/A03_explicit_object/build/local -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/A03_explicit_object/build/local --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/A03_explicit_object/build/local -C Debug --output-on-failure
```
