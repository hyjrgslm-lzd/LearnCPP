# L07 RAII r2 作者返修报告

状态：作者返修完成，等待非作者复验。未扩展 08-10，未修改公共 helper、CI、系统配置、commit 或 push。

## 返修内容

- 将资源 fixture 移到 `Core_Study/exercises/L07_raii/checks/support/resource_model.hpp`。Student 只实现 `l07::two_resource_owner`，不能定义资源计数、事件或失败注入。
- 移除 `student_ready`、placeholder gate 和 Student 自带资源报告。
- Reference 删除 `two_resource_owner(AcquisitionPlan)`，失败计划只由 checker fixture 在无 live resource 时设置。
- Reference 自定义 `two_resource_owner::operator=(two_resource_owner&&)`，先按 `reset()` 逆序释放旧 target，再接管 source，避免默认 memberwise move assignment 先释放 first。
- fixture 使用固定容量事件数组。release 路径 `noexcept`，不做 `std::string`、`std::to_string` 或 vector 分配；release 校验资源种类、id 和重复释放。
- checker 增加 first acquire 失败、second acquire 失败展开、多 owner 交错、reset 幂等、move/selfmove、move assignment 精确释放顺序和 fixture live-reset 拒绝。
- 新增 `validation/bad_fake_completed/owner.hpp`。该实现会获取资源但漏释放 second，用于回归证明旧 checker 漏掉的 fake-complete 类问题已被拒绝。
- 新增 `validation/bad_memberwise_move_order/owner.hpp`。该实现使用默认 memberwise move assignment，用于证明旧 target 释放顺序必须被检查。

## r2-final 证据

- `author-owner-l07-r2-final-build-debug.json`：Debug 默认构建 PASS。
- `author-owner-l07-r2-final-configure.json`：默认 leaf configure PASS。
- `author-owner-l07-r2-final-ctest-debug.json`：Debug 默认 CTest PASS，observation/reference 2/2。
- `author-owner-l07-r2-final-build-release.json`：Release 默认构建 PASS。
- `author-owner-l07-r2-final-ctest-release.json`：Release 默认 CTest PASS，observation/reference 2/2。
- `author-owner-l07-r2-final-student-configure.json`：`CORE_STUDY_BUILD_REFERENCE=OFF` 且 `CORE_STUDY_TEST_STUDENTS=ON` 的 Student-only configure PASS。
- `author-owner-l07-r2-final-student-build.json`：Student-only 下 observation/student 构建 PASS。
- `author-owner-l07-r2-final-student-ctest-placeholder-fails.json`：Student 占位由真实 checker 拒绝，诊断 `check failed: normal owner must own first`。
- `author-owner-l07-r2-reference-off-target-absent.json`：Reference 关闭时构建 `L07_raii_reference` 失败，诊断 `MSB1009`。
- `author-owner-l07-r2-final-validation-configure.json`：公开验证变体 configure PASS。
- `author-owner-l07-r2-final-validation-build.json`：good、bad_noop、bad_fake_completed、bad_memberwise_move_order 四个公开变体构建 PASS。
- `author-owner-l07-r2-final-validation-good-run.json`：good 运行 PASS，输出 `L07_raii_validation_contract OK`。
- `author-owner-l07-r2-final-validation-bad-noop-run.json`：bad_noop 被拒绝，诊断 `check failed: normal owner must own first`。
- `author-owner-l07-r2-final-validation-bad-fake-completed-run.json`：bad_fake_completed 被拒绝，诊断 `check failed: normal lifetime: second resource still alive`。
- `author-owner-l07-r2-final-validation-bad-memberwise-move-order-run.json`：bad_memberwise_move_order 被拒绝，诊断 `check failed: move assignment releases target old second first`。

## 剩余边界

- 本报告只覆盖 L07 leaf。
- 整课顶层仍由 root 集成。
- 非作者教学/技术/实验审查与复验尚未完成。
