# C03 F01 能力探测结果，2026-09-09

命令运行目录：`F:\CPPTrain\LearnCPP\C03_Type_Modeling_Interface_Design\exercises\F01_frontier`。叶级构建目录：`build\author-frontier`。

```powershell
python ..\..\..\C02_Objects_Lifetime_Ownership\exercises\tools\record_process.py --output ..\..\references\validation\capabilities\author-frontier-configure-20260909.json --timeout 180 --contains "Build files have been written" -- cmake -S . -B build\author-frontier -G "Visual Studio 18 2026" -A x64 -D BUILD_TESTING=ON -D TYPE_STUDY_ENABLE_FRONTIER=ON
python ..\..\..\C02_Objects_Lifetime_Ownership\exercises\tools\record_process.py --output ..\..\references\validation\capabilities\author-frontier-build-20260909.json --timeout 300 --contains "F01_proposal_tracking.vcxproj ->" -- cmake --build build\author-frontier --config Release
python ..\..\..\C02_Objects_Lifetime_Ownership\exercises\tools\record_process.py --output ..\..\references\validation\capabilities\author-frontier-ctest-verbose-20260909.json --timeout 180 --contains "100% tests passed" --contains "F01_c26_contracts" -- ctest --test-dir build\author-frontier -C Release -R ^F01_ -V
```

原始过程记录与源码绑定：

- `source-sha256-20260909.json`：第一次源码/文档 SHA256 绑定，保留历史证据。
- `source-sha256-r2-20260909.json`：本次中文化文档调整后的源码/文档 SHA256 绑定。
- `author-frontier-configure-20260909.json`：PASS，exit 0。
- `author-frontier-build-20260909.json`：PASS，exit 0。
- `author-frontier-ctest-verbose-20260909.json`：PASS，exit 0。

Configure 阶段观察到的工具链：

- CXX 编译器：MSVC `19.51.36256.0`
- 生成器：`Visual Studio 18 2026`, `x64`
- 前沿目标：MSVC `/std:c++latest`

F01 CTest 实际结果：

- `-R ^F01_` 只选择 13 个 F01 测试。
- 设施 PASS：3 个，分别是 `optional` monadic、`expected` monadic、`move_only_function`。
- 信息观察 PASS：2 个，分别是 `hardening_info`、`proposal_tracking`。它们不是设施完成证明。
- 设施 SKIP 77：8 个。
- FAIL：0 个。

| 测试 | 本机实际结果 | 含义 |
|---|---|---|
| `F01_c23_optional_monadic` | PASS | C++23 optional monadic API 可用。 |
| `F01_c23_expected_monadic` | PASS | C++23 expected 与 monadic API 可用。 |
| `F01_c23_move_only_function` | PASS | C++23 move-only callable wrapper 可用。 |
| `F01_hardening_info` | PASS observation | 只记录 hardening 宏，不触发 violation；不是设施完成证明。 |
| `F01_proposal_tracking` | PASS observation | 只确认 pattern matching 与 quantities/units 是提案跟踪项，不声明本机 `std` 支持；不是设施完成证明。 |
| `F01_c26_optional_ref` | SKIP 77 | `__cpp_lib_optional` 低于 `202506L`。 |
| `F01_c26_optional_range` | SKIP 77 | 未提供 `__cpp_lib_optional_range_support`。 |
| `F01_c26_variant_member_visit` | SKIP 77 | `__cpp_lib_variant` 低于 `202306L`。 |
| `F01_c26_copyable_function` | SKIP 77 | 未提供 `__cpp_lib_copyable_function`。 |
| `F01_c26_function_ref` | SKIP 77 | 未提供 `__cpp_lib_function_ref`。 |
| `F01_c26_indirect_polymorphic` | SKIP 77 | 未提供 `__cpp_lib_indirect` 与 `__cpp_lib_polymorphic`。 |
| `F01_c26_contracts` | SKIP 77 | 未提供 `__cpp_contracts`；只有语言语法存在时才继续记录 `__cpp_lib_contracts`。 |
| `F01_c29_virtual_contracts` | SKIP 77 | 未提供 `__cpp_contracts`；SD-6 记录 P3097R3 virtual contracts 的宏值为 `202606`。 |

Verbose 证据摘录，保留原始输出文本：

```text
F01_c23_optional_monadic: __cpp_lib_optional=202110
F01_c23_expected_monadic: __cpp_lib_expected=202211
F01_c23_move_only_function: __cpp_lib_move_only_function=202110
F01_c26_optional_ref: __cpp_lib_optional=202110; SKIP: std::optional<T&> unavailable
F01_c26_optional_range: SKIP: __cpp_lib_optional_range_support not defined
F01_c26_variant_member_visit: __cpp_lib_variant=202106; SKIP: std::variant member visit unavailable
F01_c26_copyable_function: SKIP: __cpp_lib_copyable_function not defined
F01_c26_function_ref: SKIP: __cpp_lib_function_ref not defined
F01_c26_indirect_polymorphic: SKIP: __cpp_lib_indirect not defined
F01_c26_contracts: SKIP: __cpp_contracts not defined
F01_c29_virtual_contracts: SKIP: __cpp_contracts not defined
F01_hardening_info: _MSVC_STL_HARDENING=0; __cpp_lib_contracts=not-defined
F01_proposal_tracking: pattern_matching=P2688R5 proposal tracking only; quantities_units=P2980R1/P3045R9/P4185R1 proposal tracking only
```

边界：这里的 SKIP 只表示本机工具链在当前宏/版本门槛下缺少该设施，不是课程完成证明，也不是其他编译器状态证明。Contracts 示例只在实现支持时验证合法调用路径的语法、链接和运行入口；除非后续加入单独受控 violation 测试，否则不声称验证 handler 或 observe/enforce/quick-enforce 检查模式。
