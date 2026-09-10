# A02 字段投影 DSL 样章验证证据

日期：2026-09-10。最终作者证据构建目录：`C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/revision-author-evidence` 与短路径 student 目录 `build/rae-s`。

作者原始命令证据已补充到 [`sample-projection-author-records`](sample-projection-author-records/)。其中 01-05 是 ITERATE 修改前记录；06-12、14-16、19 是修改后的有效记录；13 是长路径 student 诊断基础设施失败，17 是命令引用失败，18 的 recorder 判定为 PASS 但 `Get-FileHash` 不可用导致 `source_sha256` 为 null；13、17、18 均不作为最终通过证据。

最终有效 JSON：

- [`06-configure-after-iterate.json`](sample-projection-author-records/06-configure-after-iterate.json)：Reference/good/bad 构建树重新配置。
- [`07-build-debug-after-iterate.json`](sample-projection-author-records/07-build-debug-after-iterate.json)：Debug 构建。
- [`08-ctest-debug-after-iterate.json`](sample-projection-author-records/08-ctest-debug-after-iterate.json)：Debug CTest。
- [`09-build-release-after-iterate.json`](sample-projection-author-records/09-build-release-after-iterate.json)：Release 构建。
- [`10-ctest-release-after-iterate.json`](sample-projection-author-records/10-ctest-release-after-iterate.json)：Release CTest。
- [`11-student-configure-after-iterate.json`](sample-projection-author-records/11-student-configure-after-iterate.json)：student-only 长路径配置。
- [`12-student-build-debug-after-iterate.json`](sample-projection-author-records/12-student-build-debug-after-iterate.json)：student-only 长路径构建。
- [`14-student-configure-short-after-iterate.json`](sample-projection-author-records/14-student-configure-short-after-iterate.json)：student-only 短路径配置，用于避开 MSBuild FileTracker 长路径限制。
- [`15-student-build-debug-short-after-iterate.json`](sample-projection-author-records/15-student-build-debug-short-after-iterate.json)：student-only 短路径构建。
- [`16-student-ctest-debug-expected-fail-short-after-iterate.json`](sample-projection-author-records/16-student-ctest-debug-expected-fail-short-after-iterate.json)：student-only 短路径 CTest，预期 exit 8 且包含学生失败文本。
- [`19-source-sha-and-diagnostic-entries.json`](sample-projection-author-records/19-source-sha-and-diagnostic-entries.json)：源码 SHA 与诊断 `evidence-*.json` 入口。

## Reference/good/bad 与诊断

命令：

```powershell
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../06-configure-after-iterate.json -- cmake -S C04_Generic_CompileTime_Reflection/exercises/A02_field_projection -B C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/revision-author-evidence -G "Visual Studio 18 2026" -A x64
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../07-build-debug-after-iterate.json -- cmake --build C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/revision-author-evidence --config Debug
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../08-ctest-debug-after-iterate.json -- ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/revision-author-evidence -C Debug --output-on-failure
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../09-build-release-after-iterate.json -- cmake --build C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/revision-author-evidence --config Release
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../10-ctest-release-after-iterate.json -- ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/revision-author-evidence -C Release --output-on-failure
```

结果：

- Debug build：exit 0，生成 `A02_field_projection_reference`、`A02_field_projection_validation_good`、`A02_field_projection_validation_bad`、两个 observation。
- Debug CTest：12/12 passed。包含 reference、validation_good、validation_bad_rejected、known baseline、counterexamples，以及 unknown/duplicate/empty/invalid-id/rvalue/trailing-comma/embedded-NUL 七个诊断 case。
- Release build：exit 0。
- Release CTest：12/12 passed（边界补丁后复跑）。

Debug CTest 摘要：

```text
100% tests passed, 0 tests failed out of 12
Label Time Summary:
diagnostic     =  19.89 sec*proc (7 tests)
negative       =   0.05 sec*proc (1 test)
observation    =   0.04 sec*proc (2 tests)
reference      =   0.02 sec*proc (1 test)
validation     =   0.02 sec*proc (1 test)
```

Release CTest 摘要：

```text
100% tests passed, 0 tests failed out of 12
Label Time Summary:
diagnostic     =  16.26 sec*proc (7 tests)
negative       =   0.05 sec*proc (1 test)
observation    =   0.03 sec*proc (2 tests)
reference      =   0.02 sec*proc (1 test)
validation     =   0.01 sec*proc (1 test)
```

## Student 初态

命令：

```powershell
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../14-student-configure-short-after-iterate.json -- cmake -S C04_Generic_CompileTime_Reflection/exercises/A02_field_projection -B C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/rae-s -G "Visual Studio 18 2026" -A x64 -DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../15-student-build-debug-short-after-iterate.json -- cmake --build C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/rae-s --config Debug
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output .../16-student-ctest-debug-expected-fail-short-after-iterate.json --expect-exit 8 --contains "check failed: project returns writable references in requested order" -- ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/build/rae-s -C Debug --output-on-failure
```

结果：

- Student-only configure：exit 0。
- Student-only build：exit 0，`A02_field_projection_student.exe` 可构建。
- Student-only CTest：9/10 passed，唯一失败为 `A02_field_projection_student`，输出 `check failed: project returns writable references in requested order`，recorder 以预期 exit 8 判定 PASS。这是预期初始占位失败，不是编译失败。

## 修正记录

- 首次诊断 test 名过长时，MSBuild FileTracker 在隔离构建 `.tlog` 路径上失败；已缩短诊断 test 名。
- 隔离诊断正控缺 include path；已用 A02 自带 `validation/diagnostics/setup.cmake` 配置 `checks` 与 `validation/good` include。
- rvalue project 边界从 `T&` 改为 `T&&` 加 lvalue 约束，并增加 deleted rvalue overload；补测 `std::move(const Person)`。
- 解析补测尾逗号和嵌入 NUL；首/连续空项由 `A02_diag_empty` 覆盖。
