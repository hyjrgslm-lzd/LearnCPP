# L07 RAII r3 作者返修报告

状态：作者返修完成，等待非作者复验。未扩展 08-10，未修改公共 helper、CI、系统配置、commit 或 push。本次不删除、不覆盖任何验证 JSON。

## 返修内容

- `checks/owner_checks.hpp` 先用 `"support/resource_model.hpp"` 相对路径包含真实 fixture，再包含 `<owner.hpp>`。
- Reference、Student 和公开 validation owner 都改为相对路径包含 `checks/support/resource_model.hpp`。
- L07 CMake include 目录调整为 `checks/support` 先于实现目录。
- 新增 `validation/bad_support_shadow`，该变体在自身目录放置 `resource_model.hpp` 并尝试从 `owner.hpp` 引入它；新接线应在编译阶段以 `l07_support` 重定义拒绝该 shadow。

## r3 证据

- `author-owner-l07-r3-final-configure.json`：默认 leaf configure PASS。
- `author-owner-l07-r3-final-build-debug.json`：Debug 默认构建 PASS。
- `author-owner-l07-r3-final-ctest-debug.json`：Debug 默认 CTest PASS，observation/reference 2/2。
- `author-owner-l07-r3-final-build-release.json`：Release 默认构建 PASS。
- `author-owner-l07-r3-final-ctest-release.json`：Release 默认 CTest PASS，observation/reference 2/2。
- `author-owner-l07-r3-final-student-configure.json`：`CORE_STUDY_BUILD_REFERENCE=OFF` 且 `CORE_STUDY_TEST_STUDENTS=ON` 的 Student-only configure PASS。
- `author-owner-l07-r3-final-student-build.json`：Student-only 下 observation/student 构建 PASS。
- `author-owner-l07-r3-final-student-ctest-placeholder-fails.json`：Student 占位由真实 checker 拒绝。
- `author-owner-l07-r3-final-reference-off-target-absent.json`：Reference 关闭时 target 缺席。
- `author-owner-l07-r3-final-validation-configure.json`：公开验证变体 configure PASS。
- `author-owner-l07-r3-final-validation-build.json`：good、bad_noop、bad_fake_completed、bad_memberwise_move_order 构建 PASS。
- `author-owner-l07-r3-final-validation-good-run.json`：good 运行 PASS。
- `author-owner-l07-r3-final-validation-bad-noop-run.json`：bad_noop 被 checker 拒绝。
- `author-owner-l07-r3-final-validation-bad-fake-completed-run.json`：bad_fake_completed 被 checker 拒绝。
- `author-owner-l07-r3-final-validation-bad-memberwise-move-order-run.json`：bad_memberwise_move_order 被 checker 拒绝。
- `author-owner-l07-r3-final-validation-shadow-build-rejected.json`：bad_support_shadow 在编译阶段被重定义错误拒绝，诊断包含 `error C2011` 和 `l07_support::ResourceCounters`。

root 随后把公共 `core_configure_target` 对 C01 `check.hpp` 目录设为 BEFORE。r3 代码未改动 L07 fixture 策略，另保存 after-public 证据：

- `author-owner-l07-r3-after-public-configure.json`：默认 leaf configure PASS。
- `author-owner-l07-r3-after-public-build-debug.json`：Debug 默认构建 PASS。
- `author-owner-l07-r3-after-public-ctest-debug.json`：Debug 默认 CTest PASS，observation/reference 2/2。
- `author-owner-l07-r3-after-public-build-release.json`：Release 默认构建 PASS。
- `author-owner-l07-r3-after-public-ctest-release.json`：Release 默认 CTest PASS，observation/reference 2/2。
- `author-owner-l07-r3-after-public-student-configure.json`：公共 helper 更新后 Student-only configure PASS。
- `author-owner-l07-r3-after-public-student-build.json`：公共 helper 更新后 Student-only build PASS。
- `author-owner-l07-r3-after-public-student-ctest-placeholder-fails.json`：公共 helper 更新后 Student 占位仍由真实 checker 拒绝。
- `author-owner-l07-r3-after-public-reference-off-target-absent.json`：公共 helper 更新后 Reference 关闭时 target 缺席。
- `author-owner-l07-r3-after-public-validation-configure.json`：公开验证变体 configure PASS。
- `author-owner-l07-r3-after-public-validation-build.json`：good、bad_noop、bad_fake_completed、bad_memberwise_move_order 构建 PASS。
- `author-owner-l07-r3-after-public-validation-good-run.json`：good 运行 PASS。
- `author-owner-l07-r3-after-public-validation-bad-noop-run.json`：bad_noop 被 checker 拒绝。
- `author-owner-l07-r3-after-public-validation-bad-fake-completed-run.json`：bad_fake_completed 被 checker 拒绝。
- `author-owner-l07-r3-after-public-validation-bad-memberwise-move-order-run.json`：bad_memberwise_move_order 被 checker 拒绝。
- `author-owner-l07-r3-after-public-validation-shadow-build-rejected.json`：公共 helper 更新后，bad_support_shadow 仍在编译阶段被 `error C2011` / `l07_support::ResourceCounters` 重定义拒绝。

## 剩余边界

- 本报告只覆盖 L07 leaf。
- 整课顶层仍由 root 集成。
- 非作者教学/技术/实验审查与复验尚未完成。
