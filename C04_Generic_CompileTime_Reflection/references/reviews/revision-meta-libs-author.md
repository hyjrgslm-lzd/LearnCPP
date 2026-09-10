# C04 Mp11/Hana 作者说明

结论：Mp11/Hana 作者已补齐正文、U01 实现题、U02 观察/迁移题和本机验证证据。作者未修改公共导航；后续由root完成18—24章及U01/U02的导航和根CMake接线，集成结果见本轮增量质量报告。

接口：

- U01 使用 `c04_link_mp11(target)`，依赖 `mp11-boost-1.91.0/source/include`。
- U02 使用 `c04_link_hana(target)`，依赖 `hana-boost-1.91.0/source/include`。
- 两个短目录在 `GENERIC_STUDY_ENABLE_META_LIBS=OFF` 时早返回；ON 且缺依赖时由 `MetaLibraries.cmake` 失败。

内容覆盖：

- `chapters/23-mp11.md`：list/metafunction、type-to-type、type-to-value、meta-map、`mp_map_find` 源码、insert/replace/update、map/filter/unique/product、惰性、runtime index dispatch。
- `chapters/24-hana.md`：`type_c`、`integral_c`、编译期 string key、`hana::map`、`find`、`at_key`、运行时借用值和 MPL 历史对照。
- `exercises/U01_mp11`：Student 初态提供真实 API 占位，构建通过后因 `schema_map_t<Person>` 形状错误失败；Reference/good 独立实现；bad 演示越界索引未先检查。
- `exercises/U02_hana`：观察程序验证编译期 key/type 与运行时对象借用；迁移观察用陌生 `SensorReading` 覆盖 type schema、缺键 `find`、`at_key` 必须 key、copy map、`std::ref` 借用写回和 const map 不自动 const 被引用对象。

验证证据：

- `references/validation/revision-20260910/meta-libs-author-20260910-132412/summary.json`：U01 Debug/Release configure/build/ctest PASS，含独立陌生 `Ticket` record；U02 Debug/Release configure/build/ctest PASS；U01 Student-only configure/build PASS 且 ctest 按预期失败；U01/U02 `GENERIC_STUDY_ENABLE_META_LIBS=OFF` 早返回 PASS。
- `references/validation/revision-20260910/meta-libs-u01-student-contract-20260910-135125/summary.json`：移除 `C04_MP11_STUDENT_INCOMPLETE` 和 `student_incomplete` 后，U01 默认 Debug/Release configure/build/ctest PASS；Student-only Debug/Release configure/build PASS，CTest 因 `schema_map_t<Person> must expose the Person fields before the full Mp11 contract can run` 按预期失败；陌生 `Ticket` 仍在完整合同里验证。
- `references/validation/revision-20260910/meta-libs-u02-migration-20260910-134300/summary.json`：U02 Debug/Release configure/build/ctest PASS，CTest 实际运行 `U02_hana_record_observation` 与 `U02_hana_migration_solution` 两个观察目标。
- `references/validation/revision-20260910/meta-libs-u02-migration-deleted-evidence-note-20260910-135125/deletion-note.json`：记录误删的 U02 `134229` raw/build 范围、原因和替代完整 PASS 证据；未重造旧数据。
- `references/validation/revision-20260910/dependencies-cmake-20260910-123814/summary.json`：依赖模块 ON 缺依赖、stale marker、target 污染等负例 PASS。

源码导读边界：

- 固定 Mp11 commit：`b94b089d4ec83cd397f20958f34edf25bc3e06f4`。
- `mp_map_find` 当前 MSVC 路径是 `mp_inherit<mp_identity<T>...>` + 重载解析 + 未求值 `decltype`。
- GCC 14+ workaround 分支不是本机 MSVC 路径。

