# C04 Mp11/Hana 修订复审 r2

结论：APPROVE。前次两个阻塞项已经补齐：顶层 README 接入 18-24 章和 U01/U02，U02 从纯观察补成明确的 `SensorReading` 迁移观察；新增发现的 U01 Student 宏捷径也已移除，当前初态是可编译的真实 API 占位，并由 schema 形状 guard 产生可诊断失败。

## Files Reviewed

- `C04_Generic_CompileTime_Reflection/README.md`
- `C04_Generic_CompileTime_Reflection/chapters/23-mp11.md`
- `C04_Generic_CompileTime_Reflection/chapters/24-hana.md`
- `C04_Generic_CompileTime_Reflection/exercises/CMakeLists.txt`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/CMakeLists.txt`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/checks/mp11_schema_checks.cpp`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/src/student/mp11_schema_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/src/reference/mp11_schema_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/validation/good/mp11_schema_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/U01_mp11/validation/bad/mp11_schema_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/U02_hana/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/U02_hana/CMakeLists.txt`
- `C04_Generic_CompileTime_Reflection/exercises/U02_hana/observations/hana_record_observation.cpp`
- `C04_Generic_CompileTime_Reflection/exercises/U02_hana/observations/hana_migration_solution.cpp`
- `C04_Generic_CompileTime_Reflection/references/reviews/revision-meta-libs-author.md`
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/meta-libs-u02-migration-deleted-evidence-note-20260910-135125/deletion-note.json`

## Stage 1 - Spec Compliance

PASS。

- 顶层路线已连接：`README.md:49` 明确列出 A01-A05、`U01 Mp11` 和 `U02 Hana`，章节搜索显示 18-24 已在阅读路线/表中出现。
- U01 Student 不再有宏捷径。`rg` 对 `C04_MP11_STUDENT_INCOMPLETE` 和 `student_incomplete` 在 U01/U02/23/24/README 范围内无命中。`src/student/mp11_schema_tools.hpp:46-70` 保留真实 API 名称，初态 `schema_map_t<Record>` 为空，`visit_field_type` 在 `:107` 抛 `std::out_of_range`。
- U01 checker 依赖真实 schema 形状。`checks/mp11_schema_checks.cpp:55-72` 做 `record_shape_ready`/`schema_shape_ready`，`:156-157` 用 `check(shape_ready, "schema_map_t<Person> must expose...")`，没有 `#ifdef check(false)`。
- U01 完整合同仍覆盖关键 Mp11 语义。`23-mp11.md:70-92` 明确 `mp_map_find` 缺键 `void`、insert 不覆盖、replace 覆盖/追加、update 用旧值；`:138-149` 明确 `mp_with_index` 前先检查边界；`:151-163` 对 `mp_map_find` 源码有连续推导。
- U01 bad 是安全负例。`validation/bad/mp11_schema_tools.hpp:149-150` 把越界索引折回 0 再进 `mp_with_index`，不会靠 UB 或 disabled assert；CTest 中 `U01_mp11_validation_bad_rejected` 明确拒绝。
- U02 迁移任务已具体化。`U02_hana/README.md:11-17` 要求 `SensorReading`、`type_schema`、copy map、`std::ref` borrowed map、const map 借用边界；`:19-23` 给出验收条件。
- Hana 类型/值边界清楚。`24-hana.md:5` 明确运行时值不会生成新 C++ 类型，`:55` 说明 runtime value map 是借用且必须短于对象生命周期，`:77` 说明 const Hana map 不自动 const 被引用对象。
- U02 可运行迁移实现覆盖验收。`hana_migration_solution.cpp:19` 使用陌生 `SensorReading`，`:57-69` 覆盖 copy map 不写回和 `std::ref` 写回，`:80-81` 覆盖 const map 对 borrowed value 的边界。
- 历史证据缺口已诚实记录。`deletion-note.json:5-11` 记录误删 `134229` raw/build、原因和替代 `134300` 证据；本次批准不声明该旧 raw 仍保留。

## Stage 2 - Quality / Security

PASS，未发现阻塞问题。

- `rg` 未发现宏完成标记、空 `catch`、明显 hardcoded secret 赋值。
- 未发现新增 broad fallback 掩盖失败。U01 Student 初态失败是明确 schema guard；U01 bad 负例保留错误行为并由验证拒绝；U02 删除证据用 note 明确 gap，没有重造旧数据。
- `lsp_diagnostics` 和 `ast_grep_search` 当前工具面不可用；用 MSVC build、CTest、`rg` 模式检查和 `git diff --check` 替代，不能声称 LSP 通过。

## Validation

- `r2-u01-full-configure.json`：PASS。
- `r2-u01-full-build-debug.json` / `r2-u01-full-build-release.json`：PASS。
- `r2-u01-full-ctest-debug.json` / `r2-u01-full-ctest-release.json`：PASS，4/4：`U01_mp11_reference`、`U01_mp11_validation_good`、`U01_mp11_validation_bad_rejected`、`U01_mp11_missing_key_diagnostic`。
- `r2-u01-student-configure.json` / `r2-u01-student-build-debug.json`：PASS。
- `r2-u01-student-ctest-debug-expected-fail.json`：PASS，预期 exit 8，并包含 `schema_map_t<Person> must expose the Person fields`。
- `r2-u02-configure.json`、Debug/Release build、Debug/Release ctest：PASS，CTest 2/2：`U02_hana_record_observation` 与 `U02_hana_migration_solution`。
- Scoped `git diff --check`：PASS。

## Issues

[LOW] `C04_Generic_CompileTime_Reflection/references/reviews/revision-meta-libs-author.md:3` - 首段仍写“root 仍需把 23/24、U01/U02 接入导航和根 CMake”，但当前 `README.md` 和 `exercises/CMakeLists.txt` 已接入，且同文件后续验证段也说明已补齐。  
Fix: 如果该作者说明会作为最终交付说明复用，更新这一句，避免读者误以为还有导航/CMake 阻塞。

## Recommendation

APPROVE。无 CRITICAL/HIGH/MEDIUM 阻塞。唯一 LOW 是作者说明的陈旧交接句，不影响当前 Mp11/Hana 教学代码、练习合同或验证放行。
