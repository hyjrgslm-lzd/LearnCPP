# C04 revision core 作者记录

日期：2026-09-10
范围所有者：`/root/fix_core`
本轮工具打开 Reference 前观察到的 HEAD：`f261bea`

## 改动

- 将 L07-L10 `validation/good` 头文件从 `../../src/reference/...` include 改为本轮候选实现。
- 修正 L10 Reference parser：完整消费字符串字面量载荷（`N - 1`），拒绝空输入、内嵌 NUL、空白、符号、非数字和大于 `INT_MAX` 的值。
- 为 L10 增加前导零和 `INT_MAX` 正例覆盖。
- 为 L10 增加空串、内嵌 NUL、符号、空白和溢出 diagnostic source。
- 更新 L10 README 和章节正文，说明严格 parser 契约。

## 独立性记录

- 本轮工具操作在写入 L07-L10 `validation/good` 前只读取了 Student、README、checks、CMake 和章节材料；`src/reference` 头文件是在 `core-04-independent-good-hashes.json` 记录后才由本轮工具打开。
- 但该 agent fork 继承了完整会话上下文，旧对话中已经包含 L08/L10 Reference 片段。因此这里不能声明“完全盲写”或“完全遮答案”。
- 当前 L07-L10 `validation/good` 是独立接线候选：它们不再 include Reference，且通过了本轮构建/CTest/静态 include 检查。真正盲写将由主线程另派 `fork_turns=none` 的新上下文完成。
- `core-01` 到 `core-03` 是 recorder 失败尝试，原因分别是旧版 Windows PowerShell 缺少可用 `Get-FileHash` 和控制台路径换行。它们作为真实过程证据保留；`core-04` 是通过的哈希记录。

## 验证

- `core-09-helper-hashes.json`：共享 helper 更新后，绑定当前 `StudySetup.cmake` 和 `compile_case.py` SHA256。
- `core-10` 到 `core-13`：L07-L10 Debug reconfigure 通过。
- `core-18` 到 `core-21`：L07-L10 Debug build 通过。
- `core-22`、`core-24`、`core-30`、`core-31`：L07-L10 Debug CTest 通过。L08/L10 使用 x64 叶级 build dir，确保 diagnostic runner 收到非空 platform。
- `core-32` 到 `core-39`：L07-L10 student-on configure/build 通过。
- `core-40` 到 `core-43`：L07-L10 student-on CTest 按预期失败，失败文本来自 checker，说明 Student 仍是可构建但有意未完成。
- `core-44` 到 `core-51`：L07-L10 Release build 和 CTest 通过。
- `core-52-good-no-reference-include.json`：`rg` 未在 L07-L10 `validation/good` 中发现 Reference include。

## Review 状态

作者侧验证通过。按更大修订计划，仍需非作者 review；完全盲写 good 由新上下文另行完成。
