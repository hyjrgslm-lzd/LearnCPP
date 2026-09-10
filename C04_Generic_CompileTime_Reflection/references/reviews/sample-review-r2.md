# C04 L11 样章非作者复验 r2

结论：APPROVE。仅放行 L11 样章门；不代表 C04 其他章节、Release/ASan/frontier 或整课集成完成。

审查时间：2026-09-09。复验范围为 `chapters/11-customization-points.md`、`exercises/L11_customization/**`（排除 build）、`references/validation/sample-author/summary.md`，并只读核对共享 `exercises/cmake/StudySetup.cmake`、`exercises/tools/compile_case.py`。未修改被审文件，未触碰 C05 或根导航。

## 原阻断复验

[PASS][实验/技术] 原 r1 阻断“成员存在但不可调用时，仍选择合法 ADL”已关闭。`checks/read_value_checks.cpp:58` 增加 `NeedsArgumentButHasAdl`，其 `read_value(int)` 不是零参数合法 member；`checks/read_value_checks.cpp:64` 提供同 namespace ADL `read_value(NeedsArgumentButHasAdl&) noexcept`。checker 在 `checks/read_value_checks.cpp:129` 验证返回类型为 `int&`，在 `checks/read_value_checks.cpp:136` 验证可读，在 `checks/read_value_checks.cpp:223` 到 `226` 验证返回引用身份和写回原对象，在 `checks/read_value_checks.cpp:239` 到 `243` 验证选择该 ADL 路径后的 `noexcept`。这能拒绝“看见同名 member 就停止、不再走 ADL”的错误实现。

[PASS][教学/实验] 原 r1 阻断“只给正确 baseline 和最终 Reference，没有改进前最小实验”已关闭。`CMakeLists.txt:13` 注册 `L11_customization_counterexamples`。`counterexamples.cpp:17` 到 `20` 构造 `auto` 返回丢引用反例，并在 `counterexamples.cpp:85` 到 `89` 证明修改返回值不影响原对象、返回类型不是 `int&`。`counterexamples.cpp:29` 到 `32` 构造忘记 `std::forward` 的 wrapper，并在 `counterexamples.cpp:91` 到 `94` 证明右值成员调用从 `int&&` 退成左值重载 `int&`。`counterexamples.cpp:42` 到 `45` 构造写死 `noexcept` wrapper，并在 `counterexamples.cpp:96` 到 `98` 只检查异常规格承诺和底层表达式不一致，不实际触发 `std::terminate()`。`counterexamples.cpp:47` 到 `76` 构造 ADL 污染安全模型，`counterexamples.cpp:100` 到 `104` 证明无路径对象被错误接受并回到 CPO 自己；正文 `11-customization-points.md:115` 明确这是安全观察，不宣称跑到真实栈溢出。

[PASS][报告一致性] `sample-author/summary.md:1` 起已改为中文记录；`sample-author/summary.md:21` 到 `22` 保留 r2 历史失败：`13` 为坏 CPO 声明顺序失败，`15` 为 recorder 包 CTest stdout 匹配方式失败且 CTest 自身通过。原始日志英文未被改写。

## 独立验证

- `r2-01-configure.json`：`cmake -S C04_Generic_CompileTime_Reflection/exercises/L11_customization -B C04_Generic_CompileTime_Reflection/exercises/L11_customization/build/review-r2 -G "Visual Studio 18 2026" -A x64`，PASS。
- `r2-02-counterexamples-build.json`：只构建 `L11_customization_counterexamples`，PASS。
- `r2-03-counterexamples-run.json`：直接运行 `L11_customization_counterexamples.exe`，PASS，stdout 命中 `L11_customization counterexamples observation OK`。
- `r2-04-build-debug.json`：完整 Debug build，PASS。
- `r2-05-ctest-debug.json`：完整 CTest，PASS，5/5：reference、validation_good、validation_bad_rejected、baseline、counterexamples。
- `r2-06-student-configure.json`：Student-only configure with `-DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON`，PASS。
- `r2-07-student-build-debug.json`：Student-only Debug build，PASS。
- `r2-08-student-ctest-debug-expected-fail.json`：Student-only CTest expected exit 8，匹配 `check failed: no-path object must not satisfy c04_readable`，PASS。

`lsp_diagnostics` 与 `ast_grep_search` 当前工具面不可用；已用 MSVC build/CTest 替代类型安全验证。`rg` 扫描 hardcoded secret / silent fallback / SKIP / TODO 等模式，仅命中 Student 与 bad control 的预期 `return 0`。`git diff --check` 对本范围无输出。

## 版本绑定

以下 SHA256 排除 build 与本审查证据自身：

```text
587d24417217e6085ad73060f805414417350612be62fcc70113b29e30202342  C04_Generic_CompileTime_Reflection/chapters/11-customization-points.md
40fc000f27512408a2ffd9f6676292065aef2ee70412cdef1e31f52ad017c599  C04_Generic_CompileTime_Reflection/exercises/L11_customization/CMakeLists.txt
23af3f42ecc232a13505039556462510b0b823b076d0cdb678922bc61b4f556c  C04_Generic_CompileTime_Reflection/exercises/L11_customization/README.md
0bc7141fbe89cde0ca1e26c28a06dc76ec4287abb49e5e330cd2365a06c06872  C04_Generic_CompileTime_Reflection/exercises/L11_customization/checks/read_value_checks.cpp
272a71a21e79718cf3d48d33489bc4645d477691442beb3100d85146c3d1b294  C04_Generic_CompileTime_Reflection/exercises/L11_customization/observations/baseline.cpp
d86a009a66d30b512e1b4564a6a84fc7a7a8ad4258914bcf26c86607f9e55048  C04_Generic_CompileTime_Reflection/exercises/L11_customization/observations/counterexamples.cpp
c0ddd034b6a008fe6755226c332fbeb6fb1675c515e74f3d8fad2df4b5224ee8  C04_Generic_CompileTime_Reflection/exercises/L11_customization/src/student/read_value.hpp
31cf2f65ebc71d474e2c78219627f4da12aab4aaa5cd805c65e8561cf1eba5f7  C04_Generic_CompileTime_Reflection/exercises/L11_customization/src/reference/read_value.hpp
31cf2f65ebc71d474e2c78219627f4da12aab4aaa5cd805c65e8561cf1eba5f7  C04_Generic_CompileTime_Reflection/exercises/L11_customization/validation/good/read_value.hpp
ad359bc499fee10e2e340a15404c459dfe714f442670fcf1e5c1b32fcc23144c  C04_Generic_CompileTime_Reflection/exercises/L11_customization/validation/bad/read_value.hpp
8ff3cdc2192b5b31f050ae25ae51ff78ce5bf773a33d5ad2640110869a688b9f  C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake
a62d8b82d1d17fe4a230f088d4a6c733521c8ad78e0531660d2f6601bb0a33d5  C04_Generic_CompileTime_Reflection/exercises/cmake/expect_failure.cmake
80c6025f85bf997ef1293226e63f5377b80ef7558759bc13b3256fd5f55a0644  C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py
394b9c7fc1d621fb830d3dc1c9f95d436e68a767f86d12e33f2802585c4e6bfc  C04_Generic_CompileTime_Reflection/references/implementation-spec.md
971e2f48729fa198e41dab09b8dc8b859745661ad3f184c8e061e2a11ad5e6fc  CONTENT_REFACTORING_GUIDE.md
```

## 推荐

APPROVE。r1 两个 HIGH 阻断均已关闭，当前 L11 样章可作为 C04 后续批量专题的质量门参考。
