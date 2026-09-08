# Foundations 独立复验报告

审查者：ODR/C01 非作者独立审查线程  
复验日期：2026-09-08  
结论：APPROVE

## 范围与快照

复验范围：

- `chapters/00-build-and-debug.md`
- `chapters/01-translation-and-preprocessing.md`
- `chapters/03-object-files-and-static-libraries.md`
- `exercises/A1_build_debug/`
- `exercises/B1_preprocessor/`
- `exercises/C2_archive/`
- `references/validation/foundations/`

已读取约束：根目录 `CONTENT_REFACTORING_GUIDE.md`、`LEARNCPP_GLOBAL_PLAN.md`、`Engineering_Study/references/implementation-spec.md`，以及本批章节、练习 README、CMake/check 脚本、Starter/Reference/Student 源码、作者 validation 原始证据。

新快照绑定：

- `Engineering_Study/references/validation/foundations/review-final-source.sha256`
  - 文件自身 SHA256：`b5385989b251845a470df9d7404cfc33b33d0312f46cc17c3456342e4734f6a2`
  - 与 root 提供值一致。
  - 45 条路径全部存在，当前 hash 全部匹配。
  - `exercises/build`、`CMakeFiles`、`.vcxproj`、`.slnx`、`.obj`、`.lib`、`Testing/Temporary` 命中数为 0。
- `Engineering_Study/references/validation/foundations/review-final-evidence.sha256`
  - 文件自身 SHA256：`c3dd44d59636037bfc2aaca917e641480688378f8501f8c8639868a656dff4d7`
  - 433 条路径全部存在，当前 hash 全部匹配。
  - 未混入 build 产物路径。

旧 `abc-v8-source-fingerprints.sha256` 混入 build 生成物的问题，本轮已由 `review-final-source.sha256` 取代；旧文件仅作为历史证据保留，不再作为 source-only 快照清单。

临时复验目录：`Engineering_Study/exercises/build/foundations-independent-review/`。未修改作者源码。临时完成体和坏变体均只写入上述 review build 目录。

## 上轮阻断复验

### B1 外部 inline/ODR 阻断已关闭

修复核查：

- `Engineering_Study/exercises/B1_preprocessor/src/reference/generated_value.hpp:10` 已为 `static inline int generated_value()`。
- `Engineering_Study/exercises/B1_preprocessor/README.md:3` 明确说明 per-TU internal linkage；外部 `inline` 的不同函数体会违反 ODR。
- `Engineering_Study/chapters/01-translation-and-preprocessing.md:46` 明确说明外部 linkage inline 的不同函数体是 ODR 违规，B1 使用 `static inline` 避免该问题。

独立复验：

- `final-172021-targeted.cmd`
  - B1 Ninja Debug：`B1_preprocessor_reference` + `B1_preprocessor_preprocess`，2/2 passed，0.47 sec。
  - B1 Ninja Release：2/2 passed，0.43 sec。
- `finalvs-172226-b1.cmd`
  - B1 Visual Studio Debug：2/2 passed，0.43 sec。
  - B1 Visual Studio Release：2/2 passed，0.42 sec。

结论：关闭。

### B1 preprocess false pass 阻断已关闭

修复核查：

- `Engineering_Study/exercises/B1_preprocessor/cmake/check_preprocess.cmake:27` 对预处理输出做空白归一化。
- `Engineering_Study/exercises/B1_preprocessor/cmake/check_preprocess.cmake:34` 和 `:35` 匹配 `static inline int generated_value()` 的实际返回展开。
- `Engineering_Study/exercises/B1_preprocessor/cmake/check_preprocess.cmake:37` 至 `:42` 分别要求 `((100) + 11)` 与 `((100) + 22)` 出现在函数返回表达式中。

独立坏变体：

- 临时源：`final-172021-b1bad-src/B1_preprocessor/src/reference/generated_value.hpp`
- 变体内容：`generated_value()` 实际 `return 0;`，同时放置误导字符串 `"11 22"`。
- 命令：`final2-172130-remaining.cmd`
- 结果：`B1_preprocessor_preprocess` failed，0.15 sec；诊断明确指出 `macro_a preprocess output does not show generated_value returning ((100) + 11)`，并打印归一化文本中的 `return 0;`。

结论：关闭。

### 公共 check 失败行为阻断已关闭

修复核查：

- `Engineering_Study/exercises/include/check.hpp:6` 至 `:12` 改为 `stderr` 输出 `check failed: ...` 后 `std::exit(EXIT_FAILURE)`。
- 签名仍为 `inline void check(bool condition, std::string_view message)`，对现有 caller 无接口破坏。

独立复验：

- `final-172021-targeted.cmd`
  - A1 Starter Debug：`check failed: student compute_answer(19) must return 42`，0.24 sec，CTest 非零。
- `final2-172130-remaining.cmd`
  - A1 Starter Release：同诊断，0.24 sec，CTest 非零。
  - B1 Starter Debug：`check failed: student configured_value must use the requested preprocessing-safe value`，0.24 sec，CTest 非零。
  - B1 Starter Release：同诊断，0.25 sec，CTest 非零。
  - C2 Starter Debug：`check failed: student archive_value must provide the linked definition returning 42`，0.25 sec，CTest 非零。
  - C2 Starter Release：同诊断，0.24 sec，CTest 非零。

未再出现 timeout 或 Debug CRT 弹框挂起。结论：关闭。

### A1 observation 恒真检查已关闭

修复核查：

- `Engineering_Study/exercises/A1_build_debug/checks/observation_check.cpp:7` 至 `:9` 引入 `A1_EXPECT_NDEBUG`。
- `Engineering_Study/exercises/A1_build_debug/checks/observation_check.cpp:19` 至 `:22` 对比 CMake 期望与实际 `NDEBUG` 状态。
- `Engineering_Study/exercises/A1_build_debug/checks/observation_check.cpp:23` 至 `:28` 继续区分 Debug/Release。

独立复验：

- `final-172021-targeted.cmd`
  - A1 Ninja Debug：reference + observation 2/2 passed，0.49 sec。
  - A1 Ninja Release：2/2 passed，0.50 sec。

结论：关闭。

### source-only 指纹清单已关闭

复验：

- `review-final-source.sha256`：45 条，0 missing，0 mismatch，0 build 产物命中。
- `review-final-evidence.sha256`：433 条，0 missing，0 mismatch，0 build 产物命中。

结论：关闭。

## 补充验证

### 有效 Student 完成体

临时完成体只改声明的学生入口，不调用 Reference：

- A1 Debug good completion
  - 临时源：`final-172021-src/A1_build_debug/src/student/debug_story.cpp`
  - 实现：`a1_student::compute_answer(seed)` 返回 `(seed + 2) * 2`
  - 结果：`A1_build_debug_student` passed，0.25 sec。
- C2 Debug good completion
  - 临时源：`final-172021-src/C2_archive/src/student/archive_value.cpp`
  - 实现：`archive_value()` 返回 42
  - 结果：`C2_archive_student` passed，0.26 sec。

上轮已独立验证过的 Release good completion 仍适用：A1、B1、C2 均 passed，约 0.26 sec。

### C2 符号与负例

上轮独立复验仍适用，且 C2 本轮未改核心符号/归档逻辑：

- `short-rerun-153902.cmd`
  - `C2_archive_reference`
  - `C2_archive_symbols`
  - `C2_archive_negative_missing_definition`
  - `C2_archive_negative_direct_objects`
  - 4/4 passed，8.65 sec。
- `c2n-153902/evidence/symbols.txt`
  - `used_member.cpp.obj` 中存在 External `?archive_value@@YAHXZ (int __cdecl archive_value(void))`。
  - `C2_archive_library.lib` archive member 输出包含 `?archive_value@@YAHXZ`。
- C2 缺实现坏变体被链接阶段拒绝：`LNK2019` unresolved external symbol `?archive_value@@YAHXZ`，`LNK1120`。

### A1 MinGW/GDB

上轮独立复验仍适用：

- 命令：`a1-mingw-gdb-review.cmd`
- `g++ 13.2.0`、`gdb 14.2`
- `g++ -std=c++23 -g -O0 ...\gdb_lesson.cpp` 构建成功。
- 运行输出 `42`。
- GDB batch 命中 `main` 和 `add_offset` 断点，栈包含 `add_offset -> compute_answer -> main`，局部变量可见：`seed = 19`、`offset = 2`、`adjusted = 21`。

### 文档与平台边界

- 本地 Markdown 链接抽查范围：C01 README、三章正文、A1/B1/C2 README。
- 结果：`LOCAL_LINK_ISSUES 0`。
- 未发现把未测 ELF 写成已通过；C2 README 和 03 章把 COFF/MSVC 与 ELF/GNU 规则分开描述。
- 未发现把 `STATIC_LIBRARY`、`WORKS` 或 PTY 根因写成本批结论。

## 未重复范围

- 未重复 root 已完成的聚合 VS Release/Debug 34/34 全量集成；本轮只补剩余门禁和上轮阻断点。
- 未运行 ELF/GNU `nm`/`ar` 实验；当前材料也未声称本机已通过 ELF。
- 未运行 LSP diagnostics；当前审查工具面未提供该工具。C++ 编译诊断由 MSVC/CMake/CTest 覆盖。

## 结论

APPROVE。

上轮 3 个 HIGH 和 2 个补充问题均已由当前源码关闭，并经独立定点复验覆盖：B1 `static inline` 与正文 ODR 边界一致；B1 preprocess checker 拒绝误导数字坏变体；公共 `check()` 失败路径快速、非 timeout、诊断明确；A1 observation 不再有恒真检查；新 source-only 指纹清单排除了 build 生成物。

本批 foundations 门禁通过；后续仍应由 root 的最终集成复验覆盖完整 34 项、跨课导航和未测平台边界。
