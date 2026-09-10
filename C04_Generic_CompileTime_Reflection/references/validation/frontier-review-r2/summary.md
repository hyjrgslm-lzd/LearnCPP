# frontier review r2 validation summary

日期：2026-09-10  
复验身份：非作者审查，r2 复验  
工作区：`F:\CPPTrain\LearnCPP`  
当前 Git HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`

## 证据文件

本目录只保存 r2 非作者复验证据，未覆盖 r1 报告或作者证据。

| 文件 | 结论 |
|---|---|
| `review-r2-off-ctest.json` | exit 0；OFF build tree 未注册测试，stderr 记录 `No tests were found!!!` |
| `review-r2-on-ctest-verbose.json` | exit 0；ON build tree 12/12 CTest 通过且全部为 SKIP，stdout 保留每项 `header/macro/body` |
| `review-r2-failure-control-direct.json` | exit 1；`F01_declared_failure_control.exe` 打印 `macro=1 body=1` 后返回 1 |
| `review-r2-failure-control-ctest.json` | exit 8；CTest 将 `F01_declared_failure_control` 报为 Failed，未吞成 SKIP |

## 本机复验结果

- OFF：已复用 `build-msvc-off`，CTest exit 0，未注册任何测试。
- ON：已复用 `build-msvc-frontier`，CTest exit 0，12 个 probe 全部 SKIP；本机 MSVC 仍缺 `<meta>`，因此真实 C++26/C++29 reflection subject 未编译、未运行、未声明 PASS。
- 受控失败：已复用 `build-msvc-failure-control`，直接运行 exit 1；CTest exit 8 并报告 Failed。该结果证明“已进入主体后的非 77 失败”不会被 runner/CTest 改写为 SKIP。

## 本轮审查核对

- `annotations_probe.cpp` 不再用 `constexpr auto` 持久化 `nonstatic_data_members_of(...)` 或 `annotations_of(...)` 返回的 vector；查询只在 `consteval bool annotations_work()` 中局部消费。
- CMake 的 annotation gate 已改为最小语法/头文件正例，不再把完整 subject 当作 capability probe。
- `consteval_only_values_probe.cpp` 使用 `__cpp_consteval >= 202606L` 作为实现观察入口，正文说明 P4101R1 只给占位宏值；probe 将 runtime null 正例和 `C04_P4101_NEGATIVE_ESCAPE` 负例入口分开。
- P3385R8 probe 使用 `attributes_of`、`has_attribute`、`is_attribute`、`__cpp_impl_reflection_attributes`，未再使用不存在的 `attribute_count`，也未编造宏数值。
- GNU try-compile 与 target 标准/`-freflection` 路径一致。
- fold constraints 宏门控使用数值 `#if C04_TRY_FOLD_CONSTRAINTS`。
- `declared_failure_control_probe.cpp` 默认 OFF，显式打开时证明非 SKIP 失败路径。

## 源码指纹

```text
BE40C9239D0B9540C90ABBAF4264BBEA500BA2E276952B0EFC308BD5C741E983  C04_Generic_CompileTime_Reflection/chapters/13-reflection-model.md
C79360D1A3ED8807E976FEE4983BD14F3DB9A64A64743111B32831735DDAA23E  C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md
D956A4C296DBCA12055C19F249BFE6B42AD5C4568481B627EB1F71727DF52EB9  C04_Generic_CompileTime_Reflection/chapters/15-annotations-frontier.md
1D9795CA46C821B7A2D25A1EAABBB48C08409873D4A8CFEA7844791008C4E61E  C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt
4749CBE14B29162FE0692A5D8BAAB6AC0B7BDA98295129FE2EAFF45257697D37  C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/annotations_probe.cpp
7A33B79301D4995ACBFF40266E659DD6BF1F15E55EFDB87C42FE3044A2E78438  C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/consteval_only_values_probe.cpp
D77A547F9005BFF0B083B0454E527F26B6D6A84A6F1B8DD211F8B42F19B74451  C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/attributes_reflection_proposal_probe.cpp
274F82226044DF4DC1146CBB79AEA0ABCEA413DB2FE5FEB363DE19F39751BAC3  C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/fold_constraints_probe.cpp
CBE12947BBE7400652B6AA4F2A7DCEB81D905AD342BC858DD1DA617F66091153  C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/declared_failure_control_probe.cpp
```

## 限制

本机 MSVC 19.51 没有 `<meta>`，所以 r2 只能复验能力缺失路径、runner 行为、源码门控结构和标准/提案状态表达。真实支持 reflection/annotations/P4101/P3385 的编译器上，主体仍需作为正式成本窗口或后续支持工具链验证的一部分单独编译运行。
