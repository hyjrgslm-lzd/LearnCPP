# C04 07—10 r2 非作者复验

结论：APPROVE。

复验范围：
- 交接说明：`references/validation/metaprogramming-author/r2-author-handoff-2026-09-09.md`
- 规范：`references/BUILD_GUIDE.md`、`references/implementation-spec.md`
- 章节：`chapters/07-packs-nttp.md`、`chapters/08-type-lists.md`、`chapters/09-tuple-traversal.md`、`chapters/10-constant-evaluation.md`
- 练习与连线：root `exercises/CMakeLists.txt`，L07—L10 的 README、checker、reference/student/bad、observation、diagnostic case

当前 Git HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`

## 当前 SHA256

| 文件 | SHA256 |
| --- | --- |
| `chapters/07-packs-nttp.md` | `CCBA5E27AFCFA0E786F548B4E8362CB18392928DF2994DACAA2ADD9C54D2BBC6` |
| `chapters/08-type-lists.md` | `46BAFA4FAAA5D42FB95C03D9CD31AB4BDDDC07D8CEF6BDCF52B8CB264F3D94E5` |
| `chapters/09-tuple-traversal.md` | `F0244F23832E6D42F674B3F76172269548DA8ABFEA979B75DCEAB122890A61A7` |
| `chapters/10-constant-evaluation.md` | `AA3B8C275EB8157993DC4B029B61FED6C4340E7982D8B466D8843822124C6A13` |
| `exercises/CMakeLists.txt` | `68AEE9CF6BBFC2DC03763822AE59C7087B7C3A0B0AC4E4B6AB10B988C44467E6` |
| `exercises/cmake/StudySetup.cmake` | `3BBDE5E2E79E32E052082C9453CE51CE1BF3D60C394F2058413F128F63C92026` |
| `exercises/tools/compile_case.py` | `7DABD21A7ABB70FC8B926B2913FA739583AB655C022AAD6DA0EBB6C45D1377AD` |
| `exercises/L07_packs/checks/pack_tools_checks.cpp` | `1BFD9ABF29DE70B50C1C762AE6C699777F00C1C5D2323624EDD631207C850E42` |
| `exercises/L08_type_lists/checks/type_list_checks.cpp` | `5625047EA40FC789CD9BB40D26EAF29642449D6D3DA1D64AD7E3D3034E0B086B` |
| `exercises/L09_tuple/checks/tuple_for_each_checks.cpp` | `86D6FD123B1470B5FF9906B7478BC7D3394E0B39C4F3CF5788CCDBD85302163D` |
| `exercises/L10_constexpr/checks/constexpr_tools_checks.cpp` | `CFFCFF4CDA40E7127F3F77EA789DCE5F3711E6AAC39945D071FE3690B8FCE382` |

## 独立验证

共享 `compile_case` helper 在复验中途更新。`r2-review-01` 到 `r2-review-03` 是旧 helper 下的早期证据；正式结论绑定重新 configure 后的当前 helper 证据：

| 证据 | 命令 | 结果 |
| --- | --- | --- |
| `references/validation/meta-review/r2-review-04-reconfigure-verify-core-after-helper-fix.json` | `cmake --preset verify-core -S C04_Generic_CompileTime_Reflection/exercises` | PASS，exit 0 |
| `references/validation/meta-review/r2-review-05-build-verify-core-after-helper-fix.json` | `cmake --build .../build/verify-core --config Release` | PASS，exit 0 |
| `references/validation/meta-review/r2-review-06-ctest-verify-core-after-helper-fix.json` | `ctest --test-dir .../build/verify-core -C Release --output-on-failure` | PASS，59/59 tests passed |
| `references/validation/meta-review/r2-review-07-configure-student-after-helper-fix.json` | `cmake --preset student -S C04_Generic_CompileTime_Reflection/exercises` | PASS，exit 0 |
| `references/validation/meta-review/r2-review-08-build-student-after-helper-fix.json` | `cmake --build .../build/student --config Release` | PASS，exit 0 |
| `references/validation/meta-review/r2-review-09-ctest-student-l07-l10-after-helper-fix.json` | `ctest --test-dir .../build/student -C Release -R "L07|L08|L09|L10" --output-on-failure` | 预期失败：4 个 student 失败；4 个 observation 与 2 个 diagnostic 通过 |

诊断 case 抽查：
- `build/verify-core/diagnostics/L08_lazy_type_eager_bad/evidence-1.json`：`control` PASS，`subject` FAIL，`verdict` PASS，包含 `subject_source`、`control_source`、`input_sha256`，错误输出命中 `missing_type`。
- `build/verify-core/diagnostics/L10_invalid_digit_diagnostic/evidence-1.json`：`control` PASS，`subject` FAIL，`verdict` PASS，错误输出命中 `decimal_value accepts only decimal digits`。

未运行 `lsp_diagnostics`：当前工具面没有该工具；本轮以实际 CMake/MSVC Release configure、build、CTest 与独立 diagnostic JSON 抽查覆盖语法、连线和诊断契约。

## 原阻断关闭情况

1. CLOSED — root 未注册 L07—L10  
   `exercises/CMakeLists.txt:4-6` 已把 `L07_packs`、`L08_type_lists`、`L09_tuple`、`L10_constexpr` 纳入 root lessons；root Release CTest 在当前 helper 下 59/59 PASS。

2. CLOSED — L08 只列 map/filter/concat/unique 名称，没有算法推导  
   `chapters/08-type-lists.md:76` 解释 `map_t` 的偏特化和 `F<Ts>...` 展开；`chapters/08-type-lists.md:86-93` 给出 `filter` 空表、递归、tail 与选择关系；`chapters/08-type-lists.md:107-122` 给出 `unique_impl<Seen, Rest>` 不变量和逐步轨迹。`exercises/L08_type_lists/README.md:7-11` 把同一实现路线落实到学生任务。

3. CLOSED — L08 缺少惰性实例化正反例  
   `chapters/08-type-lists.md:153` 说明 `lazy_type_t<true, provider<int>, explosive>` 只取选中分支，eager 版本应触发 `missing_type`。`exercises/L08_type_lists/checks/type_list_checks.cpp:46` 检查正例；`exercises/L08_type_lists/observations/type_lists_observation.cpp:25-40` 展示未选分支不实例化；`exercises/L08_type_lists/CMakeLists.txt:14-18` 注册 eager 负编译诊断。独立 CTest 中 `L08_lazy_type_eager_bad` PASS，诊断 JSON 证明 control/source 未混淆。

4. CLOSED — L10 常量求值讲解过浅，缺 immediate invocation、`if consteval`、`is_constant_evaluated`、对象存储和诊断链  
   `chapters/10-constant-evaluation.md:22-24` 建立 manifestly constant-evaluated context、immediate invocation、immediate function context 的边界；`chapters/10-constant-evaluation.md:32-42` 用 `if consteval` 与 `std::is_constant_evaluated()` 区分语法位置、运行时调用和试探性常量求值；`chapters/10-constant-evaluation.md:46-67` 讲清 `vector`/`string` 的瞬态存储和不能跨越常量求值边界的存储关系；`chapters/10-constant-evaluation.md:83-85` 连接练习和 invalid digit diagnostic。L10 现在足以支撑后续 reflection：它给出了 fixed_string/NTTP 输入、consteval 解析、诊断边界、瞬态对象存储和常量/运行时分支责任，不再只是术语列表。

5. CLOSED — L10 `consteval` 版本规则与传播边界不准确  
   `chapters/10-constant-evaluation.md:24` 明确 `consteval` 调用不在任意运行时位置强制可用，而取决于 immediate invocation 和 immediate function context；`chapters/10-constant-evaluation.md:40` 明确 runtime `ch` 不能传入 `parse_digit`；`chapters/10-constant-evaluation.md:77-79` 把 C++23 作为主线，C++26 与 DR 放到边界说明，当前章节不混用 frontier 状态。

6. CLOSED — L07 承诺 `count_types`/`count_values`，但学生骨架/checker 没闭合  
   `exercises/L07_packs/README.md:7` 要求实现 `count_types<Ts...>` 与 `count_values<Vs...>`；`exercises/L07_packs/checks/pack_tools_checks.cpp:17-18` 检查 type pack 与 value pack 计数；reference 当前用 `sizeof...` 实现，Student 过滤 CTest 中 `L07_packs_student` 因后续 stub 失败，说明新增最小函数没有掩盖教学失败路径。

## 新发现

LOW — `chapters/10-constant-evaluation.md:83` 仍写 `constexpr_sum`，而 README/checker/reference 使用的是 `constexpr_vector_sum`（`exercises/L10_constexpr/README.md:13`、`exercises/L10_constexpr/checks/constexpr_tools_checks.cpp:9`、`exercises/L10_constexpr/src/reference/constexpr_tools.hpp:43`）。这是命名不一致，不影响编译和练习契约，但建议最终润色时统一为 `constexpr_vector_sum`。

## Recommendation

APPROVE。无 HIGH/MEDIUM 阻断剩余。r2 已完成全局 4.2 下限内 C04 07—10 的教学深度、练习连线、诊断负例和 Student/reference/good/bad 关系闭环。
