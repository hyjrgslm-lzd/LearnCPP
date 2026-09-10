# C04 Mp11/Hana 修订审查

结论：ITERATE。U01 Mp11 的代码契约、Reference/good 独立性、bad 负例安全性和缺键诊断都通过；U02 Hana 示例没有类型/值混淆，也正确说明运行时值是借用。但交付还差两个教学/导航补缺，不能只用测试通过数替代可学性。

## Files Reviewed

- `chapters/23-mp11.md`
- `chapters/24-hana.md`
- `exercises/U01_mp11/README.md`
- `exercises/U01_mp11/CMakeLists.txt`
- `exercises/U01_mp11/checks/mp11_schema_checks.cpp`
- `exercises/U01_mp11/src/student/mp11_schema_tools.hpp`
- `exercises/U01_mp11/src/reference/mp11_schema_tools.hpp`
- `exercises/U01_mp11/validation/good/mp11_schema_tools.hpp`
- `exercises/U01_mp11/validation/bad/mp11_schema_tools.hpp`
- `exercises/U01_mp11/validation/diagnostics/missing_key.cpp`
- `exercises/U01_mp11/validation/diagnostics/control.cpp`
- `exercises/U02_hana/README.md`
- `exercises/U02_hana/CMakeLists.txt`
- `exercises/U02_hana/observations/hana_record_observation.cpp`
- `exercises/A02_field_projection/checks/projection_schema.hpp`
- `exercises/CMakeLists.txt`
- `README.md`
- `references/reviews/revision-meta-libs-author.md`

## Issues

[MEDIUM] `C04_Generic_CompileTime_Reflection/README.md:11` - 顶层阅读顺序仍只写到 17 章，表格也只列 00-17；当前 `exercises/CMakeLists.txt` 已接入 `U01_mp11 U02_hana`，但读者从课程入口找不到 18-24，尤其找不到 `23-mp11.md` / `24-hana.md`。  
Fix: 把 18-24 加入 C04 README 的阅读顺序和章节表，并在构建/证据段明确 Mp11/Hana 属于 meta-libs 增量且验证完成后再计入交付。

[MEDIUM] `C04_Generic_CompileTime_Reflection/exercises/U02_hana/README.md:3` - README 称“观察和迁移题”，但实际任务只是运行 `U02_hana_record_observation` 并观察三点；没有要求学生迁移一个陌生 record，也没有验收条件。测试能证明示例程序正确，不能证明学习者完成了 Hana 编译期 key 与运行时借用 value 的迁移。  
Fix: 二选一：把 U02 定义收窄为纯观察题；或增加一个小迁移任务，例如为陌生 record 写 `type_schema` 与 borrowed `values` map，覆盖 `find` 缺键、`at_key` 必需键、运行时修改和生命周期说明。

## Passing Evidence

- U01 Reference 在 `required_field_t` 中保留 `mp11 map key not found` 诊断，并在 `visit_field_type` 进入 `mp_with_index` 前抛 `std::out_of_range`。
- U01 good 与 Reference 是两份独立实现；bad 在越界时折回 0，不触发 UB，也不依赖 disabled assert，ctest 通过 `U01_mp11_validation_bad_rejected` 明确拒绝。
- U01 check 覆盖陌生 `Ticket` record、缺键 `void`、insert 不覆盖、replace 覆盖、update 使用旧值、map/filter/unique/product、惰性和 runtime index 边界。
- 23 章对 `mp_map_find` 有连续源码推导：`mp_identity` 包装、`mp_inherit`、重载解析、未求值 `decltype`，并区分 MSVC 当前路径和 GCC 14+ workaround。
- 24 章和 U02 代码明确区分编译期 key/type 与运行时对象；运行时 map 存指针，说明是借用，不说成拥有。

## Validation

- Blind implementation: Debug ctest 2/2 PASS；Release ctest 2/2 PASS。
- U01 leaf via C02 `record_process.py`: configure PASS；Debug build PASS；Debug ctest 4/4 PASS；Release build PASS；Release ctest 4/4 PASS。
- U02 leaf via C02 `record_process.py`: configure PASS；Debug build PASS；Debug ctest PASS；Release build PASS；Release ctest PASS。
- Scoped `git diff --check` PASS。
- `lsp_diagnostics` / `ast_grep_search` 工具在当前工具面不可用；已用 MSVC build/CTest 和 `rg` 模式检查替代，未发现 hardcoded secret、空 catch、宽 fallback 或静默默认返回。

## Recommendation

ITERATE。补齐上面两个文档/学习契约问题后，当前 Mp11/Hana 代码和验证可以进 APPROVE。
