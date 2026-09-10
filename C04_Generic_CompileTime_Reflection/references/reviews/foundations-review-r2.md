# C04 01-06 / L01-L06 r2 非作者复验报告

结论：`APPROVE`，限于 C04 01-06 正文与 L01-L06 练习的 r2 教学、技术、leaf 实验复验。r1 原 7 项阻断均已关闭。此结论不替代完整 C04 全章、全 root diag、后续性能/前沿章节的最终交付验收。

## 输入与边界

- 工作区：`F:\CPPTrain\LearnCPP`
- 当前 Git HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`
- C04 当前状态：`git status --short -- C04_Generic_CompileTime_Reflection` 返回 `?? C04_Generic_CompileTime_Reflection/`，本次按当前工作区文件复验，未把未跟踪状态当作失败项。
- 作者冻结证据：
  - `references/validation/foundations-author/r2/freeze-report.md`
  - `references/validation/lookup-author/freeze-r2.md`
- reviewer 新证据：
  - `references/validation/foundations-review/00-current-hashes.txt`
  - `references/validation/foundations-review/01-root-configure.json`
  - `references/validation/foundations-review/02-root-build-debug.json`
  - `references/validation/foundations-review/03-root-ctest-0106-debug.json`
  - `references/validation/foundations-review/04-root-student-configure.json`
  - `references/validation/foundations-review/05-root-student-build-debug.json`
  - `references/validation/foundations-review/06-root-student-ctest-0106-debug-expected-fail.json`

当前关键 SHA 已固化在 `00-current-hashes.txt`。其中：

- `foundations-author/r2/freeze-report.md`：`5821D9B6EEF90F0E70E90188546340E549449E6AAA507984443D5082FC4F0855`
- `lookup-author/freeze-r2.md`：`8078CD1C100DA4DCAC59974A3D48191E2E418A6F75A2396A3A54B64691EA0F6E`
- `exercises/cmake/StudySetup.cmake`：`3BBDE5E2E79E32E052082C9453CE51CE1BF3D60C394F2058413F128F63C92026`
- `exercises/tools/compile_case.py`：`7DABD21A7ABB70FC8B926B2913FA739583AB655C022AAD6DA0EBB6C45D1377AD`
- `exercises/CMakeLists.txt`：`68AEE9CF6BBFC2DC03763822AE59C7087B7C3A0B0AC4E4B6AB10B988C44467E6`

## 独立 MSVC 复跑结果

使用 `cmake 4.2.3`，生成器 `Visual Studio 18 2026`，平台 `x64`，配置 `Debug`。

1. 根配置：`01-root-configure.json`，PASS，exit 0。
2. 根构建：`02-root-build-debug.json`，PASS，exit 0。
3. L01-L06 root ctest：`03-root-ctest-0106-debug.json`，PASS，`100% tests passed, 0 tests failed out of 30`。
4. Student-only 配置：`04-root-student-configure.json`，PASS，exit 0。
5. Student-only 构建：`05-root-student-build-debug.json`，PASS，`--target c04_students` 可构建。
6. Student-only 01-06 ctest：`06-root-student-ctest-0106-debug-expected-fail.json`，wrapper PASS；CTest 原始 exit 8 为预期结果，`0% tests passed, 6 tests failed out of 6`。六个失败分别落在：
   - `L01_templates_student`：`check failed: quantity_value_t reads dependent value_type`
   - `L02_deduction_student`：`check failed: by-value keeps plain int`
   - `L03_forwarding_student`：`check failed: invoke_preserving must keep lvalue argument category`
   - `L04_lookup_student`：`check failed: no-route object must not satisfy c04_inspectable`
   - `L05_overload_student`：`check failed: member describe returns member_result`
   - `L06_constraints_student`：`check failed: missing name is not field_like`

诊断型子证据也由当前 helper 写入独立 build 树：

- `build/foundations-review-r2/L01_templates/diag/missing/evidence-1.json`：PASS，漏显式实例化 provider 触发 `LNK2019`。
- `build/foundations-review-r2/diagnostics/L04_lookup_missing_typename/evidence-1.json`：PASS，匹配 `error C7510`。
- `build/foundations-review-r2/diagnostics/L05_func_partial/evidence-1.json`：PASS，匹配 `error C2768|error C2912`。
- `build/foundations-review-r2/diagnostics/L05_body_error/evidence-1.json`：PASS，匹配 `error C2039`。
- `build/foundations-review-r2/diagnostics/L06_non_dep/evidence-1.json`：PASS，匹配 `error C3861`。

当前 helper 也已复核：`c04_add_compile_case` 使用 `${CMAKE_BINARY_DIR}/diagnostics/${ARG_NAME}`，避免每个 lesson 局部长路径；`compile_case.py` 记录 `subject_source`、`control_source`、`pattern`、`compiler`、`input_sha256`，并保留 control 与 subject 进程记录，UTF-8 输出设置存在。

## 原 7 项关闭情况

| r1 项 | 责任 | r2 结论 | 证据 |
| --- | --- | --- | --- |
| C++23/P2266 `decltype(auto) return(local)` 讲法过度简化 | foundations | 已关闭 | `chapters/02-deduction.md:71-73` 区分 C++20 与 C++23 P2266，说明 move-eligible return operand 可成 xvalue、可能 `int&&` 或绑定失败；`chapters/03-forwarding-ctad.md:77` 自测同样分版本。 |
| `std::forward` 被说成只有 forwarding wrapper 才正确 | foundations | 已关闭 | `chapters/03-forwarding-ctad.md:25-36` 改为“按推导得到或保存的 cv/ref 形态恢复值类别”，并指出无推导来源或用错 `T` 会伪造值类别。 |
| L01 缺真实多 TU `extern template` / provider 边界 | foundations | 已关闭 | `chapters/01-template-model.md:91` 和 `L01_templates/README.md:19` 明确正例与漏 provider 负例；`L01_templates/CMakeLists.txt:14-26` 注册 `L01_templates_extern_template` 与 `L01_templates_missing_extern_provider`；reviewer ctest 30/30 中两个 L01 observation 均通过，漏 provider 证据匹配 `LNK2019`。 |
| L01 厘米题意与 checker 不一致 | foundations | 已关闭 | `L01_templates/README.md:5` 定义厘米 base：cm=1、m=100、km=100000，主模板默认 0；`L01_templates/checks/quantity_checks.cpp:27-32` 直接检查三种倍率与 meter 转厘米。 |
| L05 缺函数模板偏序观察与 body hard-error 边界 | lookup | 已关闭 | `L05_overload/observations/overload_observation.cpp:24-29,58,66` 增加函数模板偏序观察；`L05_overload/CMakeLists.txt:14-25` 注册 `L05_func_partial` 与 `L05_body_error`；reviewer 诊断证据分别匹配函数模板偏特化非法诊断和 body `C2039`。 |
| L06 约束 subsumption 观察被形参偏序混淆 | lookup | 已关闭 | `L06_constraints/observations/constraints_observation.cpp:46-52` 使用同形参 `rank(T)`，只让 `requires` 从 `atom_a<T>` 变到 `atom_a<T> && larger_than_two<T>`；`constraints_observation.cpp:69` 检查更强约束被选中。 |
| L01-L06 未接入共享 root | shared | 已关闭 | `exercises/CMakeLists.txt:4-11` 注册 14 个单元：L01-L11、B01、P1、F01，并汇总 `c04_students`；reviewer 根构建 PASS，L01-L06 root ctest 30/30 PASS，Student-only `c04_students` 可构建。 |

## 当前教学/技术判断

正文不再依赖标题或外链补齐核心链路。01-03 能沿正文推导模板声明/实例化、显式特化/偏特化、显式实例化、推导、转发、CTAD；04-06 能沿正文和练习推导两阶段查找、ADL、重载偏序、SFINAE immediate context、requires 约束与 subsumption。r2 对 r1 阻断项的修复都落到了可读正文、练习 checker、positive/negative observation 或 diagnostic evidence 上。

代码练习的关键路径已静态和动态闭环：

- Reference/good/bad/student 路径在 root 01-06 ctest 或 Student-only ctest 中被实际触发。
- Student 占位能独立编译，但六个 student 测试均以题意相关 checker 文本失败，说明占位不会假通过。
- 负例不是只看源文件名：`compile_case.py` 先编译 control，再要求 subject 以匹配的 MSVC 语义诊断失败；L01 漏 provider 用独立脚本验证链接错误。

## 真实未验证边界

- 未运行 C04 全部 L07-L11、B01、P1、F01 的完整诊断矩阵；本报告只覆盖父任务指定的 01-06 / L01-L06 r2 复验。
- 未做 sanitizer、性能采样或跨编译器复验；本轮证据是 Windows MSVC leaf/root 最小复跑。
- C04 目录当前仍是未跟踪工作区内容；本报告记录当前 SHA 与文件状态，但不评估提交边界或最终发布包。

Recommendation：`APPROVE` for C04 01-06 / L01-L06 r2 review gate.
