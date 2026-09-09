# F01：C03 前沿设施能力探测

本单元只验证标准库和语言前沿设施的本机可用性，不替代正文教学。源码长期保留在 `capabilities/`，每个设施一个独立文件；实现未提供时返回 77，CTest 只把对应 capability 测试标为 skip。

## 叶级运行方式

从本目录独立配置，避免跑全课目标：

```powershell
cd F:\CPPTrain\LearnCPP\C03_Type_Modeling_Interface_Design\exercises\F01_frontier
cmake -S . -B build\author-frontier -G "Visual Studio 18 2026" -A x64 -D BUILD_TESTING=ON -D TYPE_STUDY_ENABLE_FRONTIER=ON
cmake --build build\author-frontier --config Release
ctest --test-dir build\author-frontier -C Release -R ^F01_ -V
```

公共 presets 仍保持 `TYPE_STUDY_ENABLE_FRONTIER=OFF` 作为普通课程默认值。开启后，MSVC 使用 `/std:c++latest`，非 MSVC 使用 `-std=c++2c`。

## 探测规则

每个程序先打印关联 feature macro。若宏缺失或版本不足，程序返回 77。若宏满足要求，程序会使用真实标准接口、实例化关键语义、链接并运行检查；此时任何编译、链接或运行失败都是真失败，不转成 skip。

Contracts 探测分三层：`__cpp_contracts` 表示语言语法，`__cpp_lib_contracts` / `<contracts>` 表示库支持，STL hardening 是独立库策略。C26 contracts 示例只声明并运行合法调用路径，覆盖 `pre`、`post`、`contract_assert` 的语法/链接入口；它不触发 violation，因此不声称验证检查模式、handler、observe/enforce/quick-enforce 行为。C29 virtual contracts 只在 `__cpp_contracts >= 202606L` 时编译 P3097R3 增量示例。

Pattern matching 和 quantities/units 当前只做提案跟踪说明，不声明本机 `std` 支持，也不造兼容 API。

## 能力含义

| 文件 | 探测内容 |
|---|---|
| `c23_optional_monadic.cpp` | `std::optional::and_then/transform/or_else` |
| `c23_expected_monadic.cpp` | `std::expected` value/error monadic 链 |
| `c23_move_only_function.cpp` | `std::move_only_function` 的 move-only 和签名限定 |
| `c26_optional_ref.cpp` | `std::optional<T&>` rebind 语义 |
| `c26_optional_range.cpp` | `std::optional` 作为 0/1 range |
| `c26_variant_member_visit.cpp` | `std::variant::visit` 成员接口 |
| `c26_copyable_function.cpp` | `std::copyable_function` copyable 且 const-correct |
| `c26_function_ref.cpp` | `std::function_ref` 非拥有 callable reference |
| `c26_indirect_polymorphic.cpp` | `std::indirect` / `std::polymorphic` 值语义 |
| `c26_contracts.cpp` | C++26 contracts 合法调用路径的语言语法；库宏只记录 |
| `c29_virtual_contracts.cpp` | C++29 virtual contracts 增量语法 |
| `hardening_info.cpp` | STL hardening 宏状态；信息观察，不是 facility 完成证明 |
| `proposal_tracking.cpp` | pattern matching 与 quantities/units 提案边界；信息观察，不是 facility 完成证明 |
