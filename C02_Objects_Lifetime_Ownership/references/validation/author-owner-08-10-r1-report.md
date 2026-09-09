# author-owner 08-10 r1 验证报告

状态：07/L07 保持 r3 审核通过后的冻结状态；本轮新增 08/09/10 章节与 L08/L09/L10 练习。未删除或覆盖任何旧 JSON。L08/L09/L10 本轮中间失败记录全部保留。

## 范围

新增/更新：

- `Core_Study/chapters/08-unique-ownership.md`
- `Core_Study/chapters/09-shared-ownership.md`
- `Core_Study/chapters/10-control-block.md`
- `Core_Study/exercises/L08_unique/**`
- `Core_Study/exercises/L09_shared/**`
- `Core_Study/exercises/L10_control_block/**`
- `Core_Study/references/validation/author-owner-l08-r1-*.json`
- `Core_Study/references/validation/author-owner-l09-r1-*.json`
- `Core_Study/references/validation/author-owner-l10-r1*.json`

未改公共 helper、reviewer probe、CI、依赖、git history。

## 源码导读固定点

本机 MSVC STL `<memory>`：

- 路径：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\memory`
- SHA256：`8955101828FE9D46C77E214E50415BAC5B1E59697E0F59BC46FA145AA1E8AE4C`
- 上游对照：Microsoft STL `msvc-build-tools-14.51`，GitHub release commit `edd1486`

## 受信 fixture / checker 接线

- L08 checker 先 `#include "support/unique_support.hpp"`，实现头用 `../../checks/support/unique_support.hpp`。
- L09 checker 先 `#include "support/shared_support.hpp"`，实现头用 `../../checks/support/shared_support.hpp`。
- L10 checker 先 `#include "support/rc_support.hpp"`，实现头用 `../../checks/support/rc_support.hpp`。
- 三个叶级 CMake 均将 `checks/support` 放在实现目录前。
- Student 无 `student_ready`、`student_placeholder_state`、report/计数提供入口。

## Fresh 验证证据

### L08_unique

通过记录：

- `author-owner-l08-r1-configure-debug.json`：configure Debug + validation variants，exit 0。
- `author-owner-l08-r1-build-debug-msbuild-localized.json`：Debug build，exit 0。
- `author-owner-l08-r1-ctest-debug.json`：observation/reference CTest，`100% tests passed`。
- `author-owner-l08-r1-good-debug.json`：public good 输出 `L08_unique_validation_contract OK`。
- `author-owner-l08-r1-bad-noop-debug.json`：public bad noop，expected exit 1，含 `check failed`。
- `author-owner-l08-r1-bad-release-debug.json`：public bad release-keeps-owner，expected exit 1，含 `release leaves unique_ptr empty`。
- `author-owner-l08-r1-configure-release-default.json` / `build-release-default.json` / `ctest-release-default.json`：Release 默认 reference/observation 通过。
- `author-owner-l08-r1-configure-student-ref-off.json` / `build-student-ref-off-msbuild-localized.json` / `student-placeholder.json`：Reference OFF + Student build 成功，Student 占位通过同一 checker expected exit 1。

保留的中间 FAIL：

- `author-owner-l08-r1-build-debug.json` 和 `author-owner-l08-r1-build-release.json`：命令 exit 0，但 record_process required text 误写为英文 `Build succeeded`；当前 MSBuild 本地化输出不含该短语。后续用 `*-msbuild-localized.json` 与默认 Release 记录补足。
- `author-owner-l08-r1-build-student-ref-off.json`：同一 required text 误配；后续 `*-msbuild-localized.json` 补足。

### L09_shared

通过记录：

- `author-owner-l09-r1-configure-debug.json` / `build-debug.json` / `ctest-debug.json`：Debug + validation variants 构建，observation/reference CTest 通过。
- `author-owner-l09-r1-good-debug.json`：public good 输出 `L09_shared_validation_contract OK`。
- `author-owner-l09-r1-bad-cycle-debug-actual-diagnostic.json`：public bad cycle expected exit 1，含 `weak backedge does not keep cycle alive`。
- `author-owner-l09-r1-bad-weak-debug.json`：public bad weak resurrect expected exit 1，含 `lock fails after destruction`。
- `author-owner-l09-r1-configure-release-default.json` / `build-release-default.json` / `ctest-release-default.json`：Release 默认 reference/observation 通过。
- `author-owner-l09-r1-configure-student-ref-off.json` / `build-student-ref-off.json` / `student-placeholder.json`：Reference OFF + Student build 成功，Student 占位 expected exit 1。

保留的中间 FAIL：

- `author-owner-l09-r1-bad-cycle-debug.json`：bad 目标 exit 1 正确，但 required diagnostic 误写为 `weak backedge: node still alive`；实际输出 `weak backedge does not keep cycle alive`。后续 `*-actual-diagnostic.json` 补足。

### L10_control_block

最终通过记录：

- `author-owner-l10-r1d-configure-debug.json` / `build-debug.json` / `ctest-debug.json`：Debug + validation variants 构建，observation/reference CTest 通过。
- `author-owner-l10-r1d-good-debug.json`：public good 输出 `L10_control_block_validation_contract OK`。
- `author-owner-l10-r1d-bad-leak-control-block-debug.json`：public bad leak expected exit 1，含 `control block still alive`。
- `author-owner-l10-r1d-bad-weak-resurrect-debug.json`：public bad weak resurrect expected exit 1，含 `object destroyed after last strong`。
- `author-owner-l10-r1d-configure-release-default.json` / `build-release-default.json` / `ctest-release-default.json`：Release 默认 reference/observation 通过。
- `author-owner-l10-r1d-configure-student-ref-off.json` / `build-student-ref-off.json` / `student-placeholder.json`：Reference OFF + Student build 成功，Student 占位 expected exit 1。

保留的中间 FAIL：

- `author-owner-l10-r1-*.json`：首次 checker 用 `check(owner, ...)`，`rc_ptr` 只有 `explicit operator bool`，MSVC 正确拒绝非上下文隐式转换。修复为 `static_cast<bool>(owner)`。
- `author-owner-l10-r1b-*.json`：`make_rc` 不能访问 `rc_ptr` 私有 adopt 构造器。修复接线。
- `author-owner-l10-r1c-*.json`：friend 模板声明造成 public bad 变体中 `make_rc` 重载歧义；正常 Reference/Release/Student 已通过，但 bad 目标未能证明契约拒绝。修复为公开内部 adopt 构造器，r1d 全通过。

## Known gaps

- 本轮不自审、不放行后续批次；代码冻结后等待非作者 review/verifier。
- L10 是教学单线程模型，明确不提供并发、aliasing、deleter/allocator、`enable_shared_from_this`。
- L08/L09/L10 public bad 只覆盖本章核心代表性错误，不宣称穷尽所有错误实现。
