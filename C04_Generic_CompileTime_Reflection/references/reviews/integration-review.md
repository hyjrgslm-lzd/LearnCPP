# C04 集成审查准备记录

审查身份：非作者集成核对  
审查时间：2026-09-10  
结论：`READY_FOR_FINAL`

本结论只表示当前集成准备范围未发现新的阻断。它不宣称 C04 最终全部完成；最终签核仍等待主线更新 `quality-report.md`、交付 manifest、全局状态，以及成本数据终审。

## 审查范围

- 新审范围：`chapters/00-learning-route.md`、`chapters/17-source-and-bridges.md`、`exercises/cmake/StudySetup.cmake`、`exercises/tools/compile_case.py`、`exercises/CMakePresets.json`、根导航桥接、`check_navigation.py`、`freeze_delivery.py`。
- 证据核对：root Debug/Release/ASan/Student/frontier JUnit 与 process JSON，Student include audit 02，旧 include trace 01，已有 r2 非作者关闭记录。
- 明确排除：未运行 compiler/build/长测试/benchmark；未审最终成本结果；未修改 C05 或其他作者被审文件。

## 证据

- `references/validation/integration-review/navigation-01.json`：`PASS`，89 个文档、153 个本地链接，0 failures；6 个 C05 链接作为并行工作排除项单列。
- `references/validation/integration-review/evidence-summary-01.json`：复算 root JUnit 为 Debug 59/59、Release 59/59、ASan 20/20、Student 34 项中 12 个 student 初始失败且 22 项通过、frontier 72 项中 59 过 13 SKIP。
- `references/validation/root-student-audit-02.json`：`PASS`，12 个 student target，`actual_include_count=1794`，0 failures；scope 明确不是抄袭证明。
- `references/validation/root-student-include-trace-01.json`：旧记录 `status=PASS` 但 `verdict=FAIL`，只要求英文 `including file:`；不作为当前失败继承。
- `references/validation/root-student-include-trace-02.json` 与 `root-student-audit-02.json`：当前中文 include trace 问题已用 02 轮记录覆盖。
- `references/validation/compile-runner-r2-good-detail.json`：正控制先过，subject 非零退出并匹配 `error C2338`，记录 `subject_source`、`control_source`、`input_sha256`。
- `references/validation/compile-runner-r2-false-detail.json`：正例 subject 成功编译时 verdict 为 `FAIL`，证明成功编译不会冒充语义拒绝。
- `references/reviews/sample-review-r2.md`、`foundations-review-r2.md`、`metaprogramming-review-r2.md`、`record-review-r2.md`、`frontier-review-r2-delivery-contract.md`：已读到对应 r2 关闭或范围批准记录；未把旧 r1/历史失败当当前失败，也未把其批准外推到最终整课。

## 核对结论

`00-learning-route.md` 明确分开运行时对象、表达式、模板参数、反射元信息，说明 Student、Reference、observation、diagnostic 和 SKIP/PASS 的证据含义；未发现把目录存在、Reference 通过或前沿 SKIP 写成学生完成或真实反射通过。

`17-source-and-bridges.md` 绑定本机 STL 输入版本与 `source-inputs.json` SHA，源码导读限定为安装头文件，不冒称上游 Git；C06/C09/C10/C14/C15 回访只要求读者推导机制边界，未把 C04 教学 CPO 替换成 range/sender/UE 的完整协议。

共享 CMake 当前注册 12 个 `c04_add_exercise` student target；默认核心预设不注册 Student 测试，student preset 才注册并真实失败。普通测试默认 30s，diagnostic 外层 420s，`compile_case.py` 内部 configure/control/subject 分别有 60s/180s/180s 上限。

frontier 当前 13 个 SKIP 是能力缺失边界：P1 reflection 1 项 + F01 12 项，stdout 均保留 `header/macro/body` 或等价能力状态；已有 `frontier-review-r2-delivery-contract.md` 明确这是本机验证+未测范围，不是主体 PASS。

`freeze_delivery.py` 只应在最终交付快照时运行；当前审查只读其逻辑，确认它排除自身输出、记录源码 SHA、二进制元数据和 C05 并行边界。主线后续生成 manifest 后仍需末轮对齐。

## Gaps

- `quality-report.md` 仍是“实施中”旧状态；这是主线计划中的收尾，不是本轮新缺陷。
- 成本作者已记录 clock 边界后的 full PASS，但成本终审由另一位 reviewer 负责；本记录不批准性能数字。
- delivery manifest 尚未由主线最终冻结；本记录不替代最终 manifest 签核。

## Risks

- 最终质量报告若把 frontier SKIP 写成真实 C++26/C++29 反射主体 PASS，或把 Student 初始失败写成课程失败/学生完成，仍会形成发布阻断。
- 最终 manifest 生成后若 `quality-report.md`、coverage、全局计划 7.2 与本轮证据统计不一致，需要末轮修正后再签核。
