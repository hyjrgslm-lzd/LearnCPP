# C01 最终教学/导航独立审查

审查者：ODR/C01 非作者独立审查线程  
审查日期：2026-09-08  
结论：APPROVE（完整 C01 教学/导航及本批跨课衔接门通过）

## 范围与约束

本轮只读审查 C01 学习路线与教学/导航闭环：

- 根 `README.md`、`LEARNCPP_GLOBAL_PLAN.md`、`CONTENT_REFACTORING_GUIDE.md`
- `Engineering_Study/README.md`
- `Engineering_Study/chapters/00-build-and-debug.md` 至 `10-packaging-and-compatibility.md`，共 11 章
- `Engineering_Study/exercises/BUILD_GUIDE.md`
- C01 13 个练习 README：A1、B1、C1、C2、D1、E1、F1、F2、G1、G2、H1、I1、J1，另含 `G1_diagnostics/static_analysis/README.md`
- `Engineering_Study/references/coverage.md`、`quality-report.md`、`standards-and-toolchains.md`
- Coroutine/Concurrency 的 README、coverage、quality report 中 C01 桥接状态

未改作者源码或根文档。临时复现实验只写入：

- `Engineering_Study/exercises/build/foundations-independent-review/final-teaching-g1-174216/`
- `Engineering_Study/exercises/build/foundations-independent-review/final-teaching-f2-174318/`

本轮按 root 要求不跑长构建矩阵；root 的四配置 fresh matrix 与 G2 1+5 采样仍在另一流程中，不在本报告中提前批准。

## 阻断问题

### HIGH-1：G1 Student checker 接受特判 `"42"` 的坏 parser，不能证明 README 声明的 parser 契约

位置：

- `Engineering_Study/exercises/G1_diagnostics/README.md:5`
- `Engineering_Study/exercises/G1_diagnostics/README.md:9`
- `Engineering_Study/exercises/G1_diagnostics/README.md:20`
- `Engineering_Study/exercises/G1_diagnostics/README.md:48`
- `Engineering_Study/exercises/G1_diagnostics/checks/student_check.cpp:7`
- `Engineering_Study/exercises/G1_diagnostics/checks/student_check.cpp:10`

问题：

G1 README 把 Student 契约写成：`parse_two_digits` 只接受恰好两个 ASCII 十进制数字，返回 0 到 99，其他输入抛 `std::invalid_argument`。解析部分还明确说不能只靠 `"42"` 测试，因为恒定/特判实现会漏掉 `"00"` 等输入。

但 `student_check.cpp` 只检查：

- `parse_two_digits("42") == 42`
- `parse_two_digits("x")` 抛 `std::invalid_argument`

这允许学生写一个只识别 `"42"`、其他全部抛异常的实现，并通过 Student 测试。它没有验证两位数字的一般规则，也没有覆盖 README 中点名的 `"00"`、非数字两字符、长度不等于 2 等关键边界。

复现：

临时坏变体只改声明的学生文件：

- `Engineering_Study/exercises/build/foundations-independent-review/final-teaching-g1-174216/src/G1_diagnostics/src/student/parser.cpp`

坏实现：

```cpp
int parse_two_digits(std::string_view text) {
    if (text == "42") {
        return 42;
    }
    throw std::invalid_argument("not 42");
}
```

命令：

- `Engineering_Study/exercises/build/foundations-independent-review/final-teaching-g1-174216/g1-bad-special-case.cmd`

结果：

- `G1_diagnostics_student` passed，0.25 sec。
- CTest：`100% tests passed, 0 tests failed out of 1`。
- `TEST_RESULT=0`。

影响：

实现型 Student 可以在未实现 parser 能力时获得绿色结果，违反“练习验证知识，不能通过预填输出/特判结果伪造完成”的验收要求。G1 又是诊断单元，checker 本身放过这种坏实现会削弱“诊断工具检查真实契约”的教学目标。

最小修复建议：

- 扩充 `G1_diagnostics/checks/student_check.cpp`，至少覆盖：
  - 合法：`"00" -> 0`、`"09" -> 9`、`"42" -> 42`、`"99" -> 99`
  - 非法：`""`、`"x"`、`"4x"`、`"123"`
- 对非法输入逐个 catch `std::invalid_argument`，并确认未抛其他异常。
- 保留 Reference 独立；修复后用同类特判坏变体复验必须失败，且 Starter 仍快速非零失败。

### HIGH-2：F2 Student checker 接受未调用 provider、只复制公式的坏实现，不能证明“依赖消费”能力

位置：

- `Engineering_Study/exercises/F2_dependencies/README.md:37`
- `Engineering_Study/exercises/F2_dependencies/README.md:45`
- `Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp:5`
- `Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp:8`
- `Engineering_Study/exercises/F2_dependencies/CMakeLists.txt:34`
- `Engineering_Study/exercises/F2_dependencies/CMakeLists.txt:36`

问题：

F2 Student 的教学目标不是“算出同样数字”，而是“把输入交给真实 provider 并原样保留其结果”。README 也明确说源码审查要确认真正调用 provider，而不是另写一份同样公式。

当前 `student_check.cpp` 用 20、-1、0、21 四个输入防住了恒定 42，但防不住直接复制 fixture 公式 `input * 2 + 2`。该坏实现仍链接了 provider target，但没有调用 provider API；绿色结果不能证明依赖消费能力。

复现：

临时坏变体只改声明的学生文件：

- `Engineering_Study/exercises/build/foundations-independent-review/final-teaching-f2-174318/src/F2_dependencies/src/student/student.cpp`

坏实现：

```cpp
int student_use_dependency(int input) {
    return input * 2 + 2;
}
```

命令：

- `Engineering_Study/exercises/build/foundations-independent-review/final-teaching-f2-174318/f2-bad-formula-no-provider-call.cmd`

结果：

- `F2_dependencies_student` passed，0.27 sec。
- CTest：`100% tests passed, 0 tests failed out of 1`。
- `TEST_RESULT=0`。

影响：

F2 是 C01 依赖消费单元。若 Student 不调用 provider 也能通过，学生可以绕开 target usage requirements、安装/消费边界和真实依赖 API。README 里的“源码审查还要确认”不能替代自动 checker 的完成门；学习者本地跑 `ctest -L student` 会得到错误的完成信号。

最小修复建议：

- 增加可运行的调用证据，而不是只比较 provider 当前公式。可选做法：
  - provider fixture 增加一个只由 `compute_answer()` 改变的可查询调用计数/last input，仅用于教学检查；Student checker 先清零或读取前值，调用 `student_use_dependency()` 后确认 provider 被调用且输入透传。
  - 或把 Student 与 provider 链接改成只能通过 provider 暴露的不透明状态完成，避免公式复制可通过。
  - 若坚持源码审查作为门禁，应提供一个随 CTest 运行的 source/audit check，明确拒绝未出现 `f2_provider::compute_answer(input)` 的实现；这比运行期证据弱，但至少不能让普通 `ctest -L student` 误报完成。
- 修复后复验上述公式坏变体必须失败，同时有效 Student `return f2_provider::compute_answer(input);` 必须通过。

## 已核查通过的教学/导航点

以下项目未发现阻断：

- C01 顶层 README 有 11 章顺序，先修关系清楚：00 → 01/02/03 → 04/05 → 06/07 → 08/09 → 10；明确 C04/C02 等不由 C01 全量替代。
- `coverage.md` 覆盖 13 个练习单元，并把 A/B/C/D/E/F/G/H/I/J 与正文主讲位置对应起来。
- `BUILD_GUIDE.md` 区分 configure/build/run/CTest，区分 reference/observation/student/negative/capability，说明 timeout 是失败，说明 `check()` stderr+exit1，说明 Modules/import std/J1/G2 的工具与平台边界。
- 本地 Markdown 链接抽查覆盖根 README、global plan、C01 README、C01 全部 markdown、Coroutine/Concurrency 桥接 README/coverage/quality report；结果 `LOCAL_LINK_ISSUES 0`。
- B1 已说明 `static inline` 的 per-TU internal linkage 与外部 inline ODR 边界；preprocess checker 已从散落数字改为匹配实际函数展开。该点已在 foundations 独立复验中 APPROVE。
- H1 README 把观察型实验、隐藏类型可达性、global/private fragment、模块包消费和独立工作区分开；命令从 LearnCPP 根目录运行，路径与实际 leaf 结构一致；未把修改 Reference 当成 Student 完成。
- I1 README 区分 CMake 集成路径、源码/扫描/metadata/运行、直接 `cl` 路径与失败边界；未把 `#include` 或单独输出冒充 `import std` 验证。
- J1 README 区分 static/shared package consumer、prefix_A 到 prefix_B 复制、版本失败、缺 target 失败和 DLL 搜索失败；未把 ELF 路径写成 Windows 已验证。
- G2 README 区分 correctness、clean/no-op/implementation/public-header、1+5 正式采样、原始样本和不从单次耗时归因；当前正式采样结果等待 root 后续回填。
- 根 README/global plan 与 Coroutine/Concurrency quality/coverage 桥接状态保守：C01、C08、C09 的范围、未验证平台和 starter 状态分开，未把本批 C01 当作 18 课全局完成。

## 未完成/待 root 后续回填后再核对

- root 四配置 fresh matrix `references/validation/integration/final-matrix-r1` 正在运行，本轮未重复长矩阵。
- G2 静止窗口 1+5 采样与最终质量报告回填尚待 root 提供新数据后再核对。
- `Engineering_Study/references/quality-report.md` 当前仍写“实施中/不宣称整课完成”。按 root 指示，本轮不因此单独判失败；待数据与状态更新后需要最后核对一次。
- 未要求 Linux/ELF 实测；当前材料把未测平台作为未验证边界保留。

## 结论

REQUEST_CHANGES。

当前最终教学/导航门不应批准，原因不是报告暂时写“实施中”，而是两个实现型 Student 完成门可被代表坏实现绕过：

1. G1 特判 `"42"` parser 通过 Student。
2. F2 不调用 provider、复制公式通过 Student。

修复这两项后，再用相同坏变体和有效完成体复验；随后等 root 的 final matrix、G2 采样和质量报告状态回填完成，再做最终批准核对。

---

## 2026-09-08 最终 G1/F2 阻断定点复验

复验目标：只验证上一轮两个 HIGH 是否被当前冻结源码关闭；不重复 root 四配置矩阵，不启动 G2 长采样。

### 源码复核

G1 当前 Student checker 不再只测 `"42"`。`checks/student_check.cpp` 调用 `check_parse_two_digits_contract(parse_two_digits)`；共享 contract 在 `checks/parser_contract.hpp:8-39` 枚举 `"00"` 到 `"99"` 共 100 个 valid 输入，并检查空串、单字符、三字符、非数字、空白、符号、非 ASCII 数字等 invalid 输入。

F2 当前新增真实 delegation 检查。`CMakeLists.txt:39-50` 定义 `F2_dependencies_student_delegation`，把真实 `src/student/student.cpp` 和 spy provider 一起链接，不链接真实 provider。`checks/delegation_check.cpp:6-11` 检查 provider 返回值、调用次数等于 1、输入原样转发。

作者证据 `Engineering_Study/references/validation/toolchain/580-verify-g1-f2-checker-strength.stdout.txt` 显示 4/4 PASS：G1 good contract、G1 constant42 rejected、F2 good delegation、F2 formula mutant rejected。

### 独立 MSVC/Ninja 短复验

临时目录：`Engineering_Study/exercises/build/foundations-independent-review/final-teaching-fix-verify-msvc-1820/`。验证脚本只复制 `cmake`、`include`、对应练习目录到临时 `work/`，并只改临时副本的 Student 文件。

命令入口：

```cmd
cmd /c ""D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && python "Engineering_Study\exercises\build\foundations-independent-review\final-teaching-fix-verify-msvc-1820\verify_g1_f2.py""
```

结果：

- `f2-formula-rejected`：PASS。公式坏变体 `input * 2 + 2` 构建成功，真实 provider value check 通过，但 `F2_dependencies_student_delegation` 失败，stderr/stdout 含 `check failed: student must return the provider result without recomputing or adjusting it`，exit code 8，非 timeout。
- `f2-good-passes`：PASS。有效完成体 `return f2_provider::compute_answer(input);` 下 `F2_dependencies_reference`、`F2_dependencies_student`、`F2_dependencies_student_delegation` 全部通过。
- `g1-good-passes`：PASS。有效完成体通过 reference 与 student contract。
- `g1-special42-rejected`：FAIL by evidence criterion。原始坏变体只接受 `"42"`、其他抛 `std::invalid_argument`，构建成功，CTest 非零退出，但失败形态是 `Exit code 0xc0000409` 的未捕获异常，未输出 `check failed:`。这说明它不能再冒充完成，但 checker 对“valid 输入被学生函数抛异常”的路径仍不是受控诊断。

### 当前门禁结论

F2 上一轮 HIGH 已关闭：公式 mutant 不能再通过目标能力检查，good 完成体通过真实 provider 与 spy provider 两条路径。

G1 上一轮“只特判 42 可绿色通过”的 HIGH 已关闭，但留下新的教学/实验阻断：`parser_contract.hpp:13` 在 valid 输入循环中直接求值 `parse(input) == expected`，没有捕获学生实现对 valid 输入抛出的异常。对本轮指定复跑的原始坏变体，失败来自未捕获异常终止，而不是公共 `check.hpp` 的 stderr + `exit(1)` 诊断路径。这和课程前面已修正的 checker 失败行为目标不一致，也会给学习者较差的错误定位。

严重级别：HIGH。

最小修复：在 `check_parse_two_digits_contract` 的 valid 输入循环中先调用 `parse` 并捕获异常；若 valid 输入抛 `std::invalid_argument` 或其他异常，调用 `check(false, "parser must accept every two-digit ASCII input from 00 to 99")` 或更具体消息。保留 invalid 输入对 `std::invalid_argument` 的检查。修复后复验三项：

1. 原始 special-case `"42"` 坏变体构建成功、CTest 非零、输出 `check failed:`、非 timeout/非 crash；
2. constant42 坏变体仍被拒；
3. good parser 仍通过 reference 与 student。

因此本轮最终教学/导航门仍为 `REQUEST_CHANGES`，但范围已缩小为 G1 checker 的受控失败路径。F2 不再阻断。


---

## 2026-09-08 G1 受控失败路径复验闭环

root 最小修复范围：仅 `Engineering_Study/exercises/G1_diagnostics/checks/parser_contract.hpp`。当前源码核对：

- `parser_contract.hpp:13-18`：valid 输入调用包在 `try/catch(...)` 中；学生实现对有效两位数字抛异常时，转为 `check(false, "parser must not throw for valid two-digit ASCII input")`。
- `parser_contract.hpp:35-44`：invalid 输入仍要求 `std::invalid_argument`；其他异常类型转为 `check(false, "parser must reject invalid input with invalid_argument, not another exception")`。

公开复验脚本与结果：

- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.py`
- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.command.cmd`
- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.stdout.txt`
- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.meta.json`
- 原始逐步日志：`Engineering_Study/references/validation/toolchain/g1-contract-controlled-failure-r1/`

执行入口：

```cmd
Engineering_Study\references\validation\toolchain\581-verify-g1-contract-controlled-failure.command.cmd
```

执行结果：`exit_code=0`，`elapsed_seconds=31.7675222`。Debug 与 Release 共 8 个 case 全部满足预期：

- good：构建成功，`G1_diagnostics_student` 通过。
- constant42：构建成功，CTest 非零，输出 `check failed: parser must accept every two-digit ASCII input from 00 to 99`，非 timeout，未见 `0xc0000409` / `Exception` / `terminate called`。
- 原始 special42：构建成功，CTest 非零，输出 `check failed: parser must not throw for valid two-digit ASCII input`，非 timeout，未见 crash 形态。
- wrong_exception：构建成功，CTest 非零，输出 `check failed: parser must reject invalid input with invalid_argument, not another exception`，非 timeout，未见 crash 形态。

本轮结论：G1 最后阻断已关闭。结合上一节 F2 spy provider 复验，G1/F2 两个最终教学门阻断均已闭环。当前独立审查对 G1/F2 定点修复给出 `APPROVE`；后续只等 root 回填最终质量/导航状态后做只读核对，不需要重复本轮短构建或 F2 构建。


---

## 2026-09-08 最终教学/导航签署

最终判定：`APPROVE`。完整 C01 教学/导航门通过；本批 C01 与 Coroutine/Concurrency 的跨课衔接门通过。本判定只覆盖教学路线、导航、README/coverage/build guide/quality 状态一致性，以及本报告前文记录的 G1/F2 教学阻断闭环；技术与数据门由独立技术审查报告负责。

### 冻结快照核对

- 最终源码 manifest：`Engineering_Study/references/validation/snapshots/delivery/source.sha256`，251 行，文件 SHA256 为 `6d310aba7509f4d909081d9bfeb66a255e5d485d3b83c563ee694e9105242948`。
- 最终证据 manifest：`Engineering_Study/references/validation/snapshots/delivery/evidence.sha256`，3050 行，文件 SHA256 为 `4191ed568e5ec3bc5aa80d6a4284aac8b7483e09fdae2e22778d21b70940cb9e`。
- `Engineering_Study/references/delivery-manifest.md` 列出 C01 11 章、13 个练习单元、公共 CMake/check/helper、各单元源码/检查器/负例/观察材料、验证脚本与非作者审查报告；清单边界明确排除 build/out/.omx/cache、用户既有 P2 与指导文件范围。

### 最终文档与导航核对

- 根 `README.md` 将 `Engineering_Study` 标为 C01 入口，并说明 Windows 验证结果看课内质量报告；其他课程族仍只是各自入口，不把 C01 交付外推为全仓完成。
- `Engineering_Study/README.md` 说明 11 章与 13 个练习已实现，Windows 四配置和专项实验完成验证；明确 `_student` 与 `_reference` 是独立程序，观察程序成功不等于完成学习任务。
- `Engineering_Study/references/coverage.md` 覆盖 A1/B1/C1/C2/D1/E1/F1/F2/G1/G2/H1/I1/J1，且下游路线把 C01 与 Concurrency/Coroutine 的先修关系写清楚，不替代 C02/C04/C07/C13/C18 等后续课程。
- `Engineering_Study/exercises/BUILD_GUIDE.md`、13 个练习 README、11 章正文、`standards-and-toolchains.md`、`quality-report.md`、G2 `validation/measurements/README.md` 的链接导航抽查通过。排除 ignored build 副本和 validation 原始日志后，共核对 40 份交付/桥接 Markdown；扩展包含本审查报告与 measurement README 时为 43 份，`LOCAL_LINK_ISSUES=0`。
- `LEARNCPP_GLOBAL_PLAN.md` 7.2 已回填 C01 为 Windows 范围实现与验证完成，记录 11章/13练习、四配置147项、G1 Debug/Release正反复验、G2正式72轮通过，同时保留其他平台未测边界；Coroutine/Concurrency 本批补修/补充只声明其已审范围，不宣称完整全局完成。
- `Coroutine_Study` 与 `Concurrency_Study` 的 README/coverage/quality report 桥接状态与 C01 主讲边界一致：C09 主讲协程协议与生命周期，C08 主讲同步/发布/回收，C01 提供构建、链接、ABI、工具链和诊断先修；旧课补充不重写未改算法或未测平台结论。

### 最终证据状态核对

- `Engineering_Study/references/validation/integration/final-matrix-r3/report.json` 顶层 `status=PASS`；记录四个 fresh 配置：核心 VS Release、核心 VS Debug、modules MSVC/Ninja、import std MSVC/Ninja，命令记录均为 PASS。质量报告汇总为 Release 34/34、Debug 34/34、Modules 39/39、importstd 40/40，共 147 PASS。
- `Engineering_Study/references/validation/measurements/g2-formal-r1/report.json` 顶层 `status=PASS`，`schedule=72`、`rounds=72`、`summary=12`；参数为 1 warmup、5 samples、3 variants、4 scenarios。`validation/measurements/README.md` 明确 72/72 PASS、12 轮预热与 60 个正式样本、统计边界、fixture/runner 指纹和“不外推 PCH/LTO 普遍收益”的限制。
- `Engineering_Study/references/quality-report.md` 已把最终矩阵、G1/F2最终完成门、G2正式测量、delivery source/evidence 快照写入状态表；同时保留 Windows 实测、其他平台未验证、旧 Ninja ABI 探测不冒称根因、G2正式数据不隐藏失败或无收益配置等边界。
- 质量报告的最终批准入口指向本教学审查报告与独立技术/数据审查报告；本报告不替代技术审查签署，也不把技术 lane 尚未签署的内容提前改写为教学结论。

### G1/F2 阻断闭环一致性

本报告前文记录的两个教学阻断已经关闭：

- F2：独立 spy provider target 使用真实 `student.cpp`，不链接真实 provider；公式 mutant 不能通过 delegation 检查，good 完成体通过真实 provider 与 spy provider。
- G1：完整 parser contract 枚举 100 个 valid 输入和代表 invalid 输入；581 公开复验在 Debug/Release 下验证 good 通过，constant42、原 special42、wrong_exception 均构建成功后以受控 `check failed:` 非零退出，非 crash/timeout。

### 未验证边界

本轮最终签署没有重新构建矩阵、没有重跑 G2 72 轮、没有扩展到未请求的其他课程族，也没有声明 Linux/CI/其他编译器完成。Windows 之外的材料保持“可执行规格/未实测”状态。技术与数据完整性由另一非作者审查报告签署；本报告只负责教学/导航与本批跨课衔接门。
