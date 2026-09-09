# L15：接口演进与兼容性

先读 `../../chapters/15-interface-evolution-and-compatibility.md`。你只编辑 `src/student/compat_adapter.hpp`，实现 `l15::CompatClient`。

已提供的 v2 引擎在 `checks/v2_engine.hpp`：

```cpp
std::string fetch(std::string_view name, int timeout_ms = 250, Format format = Format::compact) const;
```

要求固定：

```cpp
std::string fetch(std::string_view name) const;
std::string fetch(std::string_view name, int timeout_ms) const;
std::string fetch_compact(std::string_view name) const;
```

Part 1：`fetch(name)` 保持 v1 行为，必须显式使用 timeout `1000` 和 `legacy` 格式。不能依赖 v2 默认实参。

Part 2：`fetch(name, timeout_ms)` 使用调用者显式 timeout，但仍保持 `legacy` 格式。

Part 3：`fetch_compact(name)` 是显式新入口，使用 v2 compact 行为和 timeout `250`。

Part 4：checker 还编译 v1 客户端形态，证明旧调用源码不需要知道 v2 的新增参数。

Part 5：`abi_model` 比较 v1/v2 公开 options 布局，给出 ABI 风险的可运行模型。它只证明“公开布局变化会改变二进制表面”这个模型，不证明真实旧二进制能否加载新库。

运行 Reference/good/bad：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L15_evolution -B build/author-c-l15 -G "Visual Studio 18 2026" -A x64
cmake --build build/author-c-l15 --config Release
ctest --test-dir build/author-c-l15 -C Release --output-on-failure
```

运行 Student 初始失败：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L15_evolution -B build/author-c-l15-student -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/author-c-l15-student --config Debug
ctest --test-dir build/author-c-l15-student -C Debug --output-on-failure
```

解析：默认实参绑定在调用点。兼容适配器要把旧默认值写进旧入口实现，而不是继续暴露 v2 默认值。validation_bad 只犯这一类错误：`fetch(name)` 直接调用 `engine.fetch(name)`，checker 用 `v1 default timeout remains 1000` 精确拒绝。ABI 部分只做公开布局变化模型；跨编译器、旧二进制加载和真实动态库边界仍标为未验证。
