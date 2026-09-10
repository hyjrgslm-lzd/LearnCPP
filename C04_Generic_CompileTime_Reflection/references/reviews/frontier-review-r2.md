# C04 reflection frontier review r2

## Code Review Summary

**Files Reviewed:** 18  
**Total Issues:** 0 open in the r2 scoped re-review

### By Severity

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

### Recommendation

COMMENT：r1 的 6 项阻断已按本轮证据关闭；未发现新的 r2 阻断。此结论不是“真实反射主体 PASS”，因为本机 MSVC 19.51 仍没有 `<meta>`，ON 路径 12 项全部是 SKIP。`lsp_diagnostics` / `clangd` / `ast-grep` 在本环境不可用，所以本报告不给形式化 APPROVE，只给范围内复验结论。

## 审查范围

冻结范围：

- `C04_Generic_CompileTime_Reflection/chapters/13-reflection-model.md`
- `C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md`
- `C04_Generic_CompileTime_Reflection/chapters/15-annotations-frontier.md`
- `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/**`

上下文与规范文件：

- `CONTENT_REFACTORING_GUIDE.md`
- `C04_Generic_CompileTime_Reflection/references/implementation-spec.md`
- `C04_Generic_CompileTime_Reflection/references/standards-and-implementations.md`
- `C04_Generic_CompileTime_Reflection/references/validation/reflection-author/r2-freeze-report.md`
- `C04_Generic_CompileTime_Reflection/references/validation/reflection-author/frontier-run-summary.md`

当前 Git HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`

## Stage 1 - Spec compliance

Spec compliance 通过本轮范围复验。`implementation-spec.md:52` 要求 OFF、能力 SKIP、PASS、FAIL 分开报告，并要求“已具能力但编译/运行错误仍 FAIL”。r2 现在满足这个边界：能力缺失只产生 SKIP；能力声明后进入主体的非 77 失败由受控项证明会被 CTest 报为 Failed。

`standards-and-implementations.md:7-10` 固定了 C++23/N4950、C++26/N5050、C++29/N5054/N5055 与 P3385R8 提案状态。r2 的正文和 F01 README 不再把 P3385R8 当成已采纳 C++29 标准，也不把 P4101R1 的 `__cpp_consteval` 占位提升写成已定标准值。

## r1 六项复验

1. `constexpr` 持久化 reflection/annotation vector：关闭。`annotations_probe.cpp:33-37` 现在在 `consteval bool annotations_work()` 内用局部 `auto fields`、`auto secret_annotations`、`auto visible_annotations` 消费查询结果；`reflection_query_probe.cpp:30` 同样只在 consteval 局部消费 `nonstatic_data_members_of` 结果。正文也在 `chapters/13-reflection-model.md:39`、`chapters/14-splicing-generation.md:40-49` 说明 query vector 是常量求值临时范围，需要局部消费或通过 `std::define_static_array` 物化。

2. annotation capability gate 掩盖坏 subject：关闭。`CMakeLists.txt:44-48` 的 annotation gate 是独立最小正例，只检查 `<meta>` 和 `struct [[=1]] AnnotatedType {};`。完整 subject 保留在 `annotations_probe.cpp:33-54`，不再作为能力探测输入。若 gate 通过而 subject 编译、链接或运行失败，会落到普通 FAIL 路径，不再被配置阶段误归为缺能力。

3. P4101 DR 语义：关闭，带本机限制。`consteval_only_values_probe.cpp:10` 改为用 `__cpp_consteval >= 202606L` 或显式 force 宏进入观察路径；`consteval_only_values_probe.cpp:27-31` 将 `C04_P4101_NEGATIVE_ESCAPE` 负例隔离为“非空 `^^int` 逃逸应拒绝”；`consteval_only_values_probe.cpp:33-37` 保留 runtime null 正例和非空反射留在 `constexpr` 对象中的正例。`chapters/15-annotations-frontier.md:55` 和 `exercises/F01_frontier/README.md:13` 明确 `202606L` 是当前实现观察入口，不是 P4101R1 给定标准常量；文档未再把旧的 `std::meta::info{} != ^^int` 当作 DR 全部证明。

4. P3385 attributes reflection：关闭。`attributes_reflection_proposal_probe.cpp:10` gate 使用 `__cpp_impl_reflection_attributes` 或显式 force；`attributes_reflection_proposal_probe.cpp:20-29` 使用 `std::meta::attributes_of`、`std::meta::is_attribute`、`std::meta::has_attribute`、`^^[[deprecated]]` 和 `identifier_of`。`chapters/15-annotations-frontier.md:57` 明确 P3385R8 是 attributes reflection 提案，API 名称和宏名来自提案，宏值仍是占位。未发现旧的 `attribute_count` 残留。

5. GNU try-compile flag 不一致：关闭。`CMakeLists.txt:32-38` 用 `f01_required_flags` 统一 C++26/C++29 required flags；GNU 会追加 `-freflection`。`CMakeLists.txt:89-100` 的 target compile options 也使用同一路径。

6. `defined(C04_TRY...)` 宏值错误：关闭。`fold_constraints_probe.cpp:4-11` 现在先按 force 宏定义 `C04_TRY_FOLD_CONSTRAINTS` 为 1，否则默认 0，并用 `#if C04_TRY_FOLD_CONSTRAINTS` 控制 subject 编译。宏定义为 0 时不会编译 fold-expanded constraints 主体。

## 缺能力不等于坏 subject 的边界

该边界已按 r2 源码和 runner 证据关闭。

- ON 普通路径：`review-r2-on-ctest-verbose.json` 记录 12/12 SKIP，且每项 stdout 都保留 `header/macro/body`。例如 `F01_consteval_only_values` 为 `header=0 macro=0 body=0`，`F01_attributes_reflection_proposal` 为 `header=0 macro=0 body=0`，`F01_fold_constraints` 为 `macro=201603 body=0`。
- 受控 FAIL 路径：`review-r2-failure-control-direct.json` 记录直接运行 exit 1，stdout 为 `probe=declared_failure_control header=na macro=1 body=1` 和 `FAIL declared capability control reached the subject body`。`review-r2-failure-control-ctest.json` 记录 CTest exit 8，测试 `F01_declared_failure_control` 为 `***Failed`。

这说明当前 runner 只把 exit 77 当 SKIP；主体已启用后的失败不会被吞掉。仍需保留一句限制：本机没有 `<meta>`，所以真实 reflection/annotation/P4101/P3385 subject 没有被本机编译器执行验证。

## Stage 2 - Code quality and security

已做静态文本扫描：

```text
rg -n 'SECRET|PRIVATE_KEY|apiKey|token\s*=|password\s*=|catch\s*\([^)]*\)\s*\{\s*\}|attribute_count|defined\(C04_TRY|constexpr auto .*nonstatic_data_members_of|constexpr auto .*annotations_of|return 77|return 1' C04_Generic_CompileTime_Reflection/exercises/F01_frontier C04_Generic_CompileTime_Reflection/chapters/13-reflection-model.md C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md C04_Generic_CompileTime_Reflection/chapters/15-annotations-frontier.md
```

结果：未发现 secret、空 catch、`attribute_count`、旧 `defined(C04_TRY...)`、`constexpr auto` 持久化 reflection/annotation vector。命中的 `return 77` 是 capability SKIP 约定；命中的 `return 1` 是真实失败路径或受控失败项，符合 spec。

`where.exe clangd`、`where.exe ast-grep`、`where.exe sg` 均为 NOT_FOUND；因此没有运行 LSP diagnostics 或 AST grep。本报告按可用工具给 COMMENT，不给形式化 APPROVE。

## 本机验证

证据目录：`C04_Generic_CompileTime_Reflection/references/validation/frontier-review-r2/`

| 命令 | 结果 |
|---|---|
| `ctest --test-dir .../build-msvc-off -C Release --output-on-failure` | exit 0；无 tests |
| `ctest --test-dir .../build-msvc-frontier -C Release -V` | exit 0；12/12 SKIP，0 failed |
| `.../build-msvc-failure-control/Release/F01_declared_failure_control.exe` | exit 1；受控主体失败 |
| `ctest --test-dir .../build-msvc-failure-control -C Release -R F01_declared_failure_control -V` | exit 8；CTest 报 Failed |

作者 r2 证据也被读取并交叉核对：`r2-freeze-report.md` 和 `frontier-run-summary.md`。作者记录的 MSVC configure 实际输出为 19.51.36256.0；summary 顶部文字写 19.51.36231，这更像工具集/编译器小版本记法差异。本轮不把它列为阻断，因为复验命令和 source hash 与当前文件一致，且该差异不影响 capability/runner 结论。

## 官方依据

- P4101R1：consteval-only values 将模型从 consteval-only type 转向 consteval-only value；null reflection 与非空 reflection 的 runtime 处理不同；feature-test macro 文字是提升 `__cpp_consteval` 到占位值。https://wg21.link/p4101r1
- P3385R8：attributes reflection 提案 API 包括 `attributes_of`、`has_attribute`、`is_attribute`，宏名为 `__cpp_impl_reflection_attributes`，值仍为 `2026XXL` 占位。https://wg21.link/p3385r8
- P3394R4：annotations 使用 `[[= constant-expression]]`，查询路径与 attributes reflection 分开。https://wg21.link/p3394r4
- P3491R3：`define_static_string/object/array` 是 `std::define_static_*`，不是 `std::meta::define_static_*`。https://wg21.link/p3491r3

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

## Stop condition

本轮 r2 要求的 6 项复验已覆盖；未修改被审源码、未触碰 P1/C05/全局文件、未派生。剩余未验证项只有真实支持 `<meta>`/reflection/P4101/P3385 的编译器主体验证，这超出当前本机 MSVC 能力窗口。
