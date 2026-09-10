# F01 frontier run summary

运行日期：2026-09-10。工具链：MSVC 19.51.36231，Visual Studio 18 2026 generator，x64，Release。

本文件保留早期失败证据文件，不覆盖旧日志；r2证据文件全部以`r2-`开头。r2修复点来自`references/reviews/frontier-review.md`：反射/annotation query vector只在`consteval`局部消费或经`std::define_static_array`物化；CMake能力探测拆成最小正例和真实主体；P4101按`__cpp_consteval` DR观察入口处理；P3385使用`attributes_of`、`has_attribute`、`is_attribute`和`__cpp_impl_reflection_attributes`；GNU try-compile和target共享必要`-freflection`；fold门控改为数值`#if`。

## 默认OFF

| 步骤 | 证据文件 | 结果 |
|---|---|---|
| configure | `r2-f01-off-configure-01.json` | PASS；stdout含`F01_frontier tests are OFF` |
| build | `r2-f01-off-build-01.json` | PASS；无frontier target需要构建 |
| ctest | `r2-f01-off-ctest-01.json` | PASS；未注册任何test |

## frontier ON

| 步骤 | 证据文件 | 结果 |
|---|---|---|
| configure | `r2-f01-frontier-configure-01.json` | PASS；`F01_HAS_ANNOTATIONS`、`F01_HAS_FOLD_EXPANDED_CONSTRAINTS`、`F01_HAS_TEMPLATE_NAME_PACK_INDEXING`最小/行为探测均失败，保持各自SKIP路径 |
| build | `r2-f01-frontier-build-01.json` | PASS；12个正常probe均生成Release exe |
| ctest | `r2-f01-frontier-ctest-01.json` | PASS；12/12测试通过，0 failed，12 skipped；每项stdout记录`header/macro/body` |

## 逐项能力（r2 MSVC）

| probe | header | macro | body | CTest |
|---|---:|---:|---:|---|
| `F01_reflection_query` | 0 | 0 | 0 | SKIP |
| `F01_splicing_generation` | 0 | 0 | 0 | SKIP |
| `F01_annotations` | 0 | 0 | 0 | SKIP |
| `F01_expansion_statement` | n/a | 0 | 0 | SKIP |
| `F01_define_static` | 0 | 0 | 0 | SKIP |
| `F01_define_aggregate` | 0 | 0 | 0 | SKIP |
| `F01_pack_indexing` | n/a | 0 | 0 | SKIP |
| `F01_fold_constraints` | n/a | 201603 | 0 | SKIP |
| `F01_constexpr_exceptions` | n/a | 0 | 0 | SKIP |
| `F01_c29_template_name_pack_indexing` | n/a | 0 | 0 | SKIP |
| `F01_consteval_only_values` | 0 | 0 | 0 | SKIP |
| `F01_attributes_reflection_proposal` | 0 | 0 | 0 | SKIP |

解释：本机没有`<meta>`且MSVC未定义N5050所需`__cpp_impl_reflection 202603L`、`__cpp_expansion_statements 202506L`、`__cpp_pack_indexing 202311L`、`__cpp_constexpr_exceptions 202411L`。`__cpp_fold_expressions`值为201603L；P2963R3行为探测未通过。C++29模板名pack indexing用MSVC可用的`/std:c++latest`探测，未通过。P4101R1在本课中按当前实现观察入口`__cpp_consteval >= 202606L`门控；本机缺`<meta>`且无该DR标记，所以DR主体未测，不冒称PASS。

## 受控FAIL路径

| 步骤 | 证据文件 | 结果 |
|---|---|---|
| configure | `r2-f01-failure-control-configure-01.json` | PASS；额外启用`GENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL=ON` |
| build | `r2-f01-failure-control-build-01.json` | PASS；生成`F01_declared_failure_control.exe` |
| direct run | `r2-f01-failure-control-run-01.json` | PASS记录；命令期望exit 1，stdout含`FAIL declared capability control` |
| ctest | `r2-f01-failure-control-ctest-01.json` | PASS记录；命令期望CTest exit 8，测试名匹配`F01_declared_failure_control` |

控制项不使用前沿语法，不证明任何C++26能力。它只证明：当测试体已经启用并返回非77失败码时，记录器和CTest不会把失败改写成SKIP。

## 旧失败证据保留

早期frontier构建失败已保留：`f01-frontier-build-01.json`和`f01-frontier-build-02.json`暴露pack/C29宏门控错误；`f01-frontier-build-03.json`暴露annotation字段attribute门控错误；`f01-frontier-build-04.json`、`f01-frontier-build-05.json`、`f01-frontier-ctest-02.json`记录r1修复后的初次通过。r2不覆盖这些文件。

## 来源核对

- N5050表22：`__cpp_constexpr_exceptions 202411L`、`__cpp_expansion_statements 202506L`、`__cpp_impl_reflection 202603L`、`__cpp_pack_indexing 202311L`；`__cpp_fold_expressions`仍为`201603L`。https://wg21.link/n5050
- P2996R13：`nonstatic_data_members_of(info, access_context)`、`enumerators_of`等query返回`vector<info>`，`identifier_of`/`type_of`有前提，splicing语法为`obj.[:m:]`。https://wg21.link/p2996r13
- P3491R3：`define_static_string`、`define_static_array`、`define_static_object`位于namespace `std`；`define_static_array`返回`span<const range_value_t<R>>`。https://wg21.link/p3491r3
- P3394R4：annotations使用`[[= constant-expression]]`，查询API含`annotations_of`、`annotations_of_with_type`、`extract<T>`、`is_annotation`。https://wg21.link/p3394r4
- P3068R6：constexpr exceptions提案线索；probe按N5050宏值`202411L`判断。https://wg21.link/p3068r6
- P4101R1：DR关联`__cpp_consteval`提升；本课不使用`__cpp_impl_reflection`冒充DR标记。https://wg21.link/p4101r1
- P3385R8：attributes reflection提案API含`attributes_of`、`has_attribute`、`is_attribute`，宏名为`__cpp_impl_reflection_attributes`且论文中仍为占位值。https://wg21.link/p3385r8
