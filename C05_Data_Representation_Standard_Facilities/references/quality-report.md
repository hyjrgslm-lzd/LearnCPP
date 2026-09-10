# C05 交付与质量报告

实施与验证日期：2026-09-09—10。范围为19章、15个专题练习、P1资源清单包、U01 ICU扩展、F01前沿，以及约定的根/C03/C09/C15导航。本文汇总真实证据，最终非作者交付结论见[最终核验](reviews/final-delivery-review.md)。

2026-09-10 fmt/spdlog增量入口：[revision-quality-report-20260910.md](revision-quality-report-20260910.md)。旧33/34项矩阵是C05既有历史基线；第19/20章、U02/U03、fmt/std backend验证和审查修复以增量报告为准，等待root最终矩阵后再合并最终总数。

## 1. 范围、环境与版本

主线C++23；Windows11 10.0.26100、AMD Ryzen9 9900X、MSVC编译器19.51.36256.0（Tools14.51.36231）、STL145/update202604、CMake4.2.3、VS2026 x64。安装输入和头文件SHA见[环境输入](validation/environment-input.json)，实际编译/链接选项在下列各构建JSON及生成工程中。安装文件存在不代替运行结果。

ICU4C77.1固定commit`457157a92aa053e632cc7fcfd0e12f8a943b2d11`，隔离头/库/DLL与版本探针见[准备说明](icu-build.md)、[最终依赖准备记录](validation/icu-prepare-20260909-231724/summary.json)。U01运行打印ICU77.1、Unicode16.0。标准库时区实验使用本机实际tzdb`2022g.27`；它不因U01使用ICU77.1而自动升级成ICU附带的时区数据。

上游STL源码导读固定commit`4edbc1d63a1ec156bed6dbf1727f413fa682abba`，四个源文件输入SHA另记[源码输入](validation/upstream-stl-inputs.json)。源码与本机安装头文件身份分开。规范使用N4950/N5050/N5054及对应编辑报告，详见[规范索引](standards-and-implementations.md)。

开始时已有C04目录和多处未提交导航/指南改动，见[输入基线](validation/input-baseline.json)。本次保留它们；没有修改旧课算法、学习文件、通用指南、机器配置或全局安装，也未提交/推送。没有性能排名；命令耗时不当作算法benchmark。

## 2. 最终fresh验证矩阵

以下均在新的`exercises/build/delivery-*-r1`目录配置和构建，使用当前最终实现；不是把作者早期缓存结果拼成最终通过。JSON保存精确命令、cwd、stdout/stderr、退出/超时/清理状态，XML保存每项结果。汇总可复核于[矩阵JSON](validation/final-matrix-summary-r1.json)。

| 配置 | 实际结果 | 原始入口 |
|---|---|---|
| 核心Release | 33 PASS，0 FAIL，0 SKIP | [构建](validation/final-core-release-build-r1.json)、[CTest](validation/final-core-release-ctest-r1.json)、[逐项XML](validation/final-core-release-r1.xml) |
| 核心Debug | 33 PASS，0 FAIL，0 SKIP | [构建](validation/final-core-debug-build-r1.json)、[CTest](validation/final-core-debug-ctest-r1.json)、[逐项XML](validation/final-core-debug-r1.xml) |
| ASan RelWithDebInfo安全路径 | 17 PASS，0 FAIL，0 SKIP | [构建](validation/final-asan-relwithdebinfo-build-r1.json)、[CTest](validation/final-asan-relwithdebinfo-ctest-r1.json) |
| frontier Release，ICU关闭 | 33 PASS，9明确能力SKIP，0 FAIL | [构建](validation/final-frontier-release-build-r1.json)、[逐项XML](validation/final-frontier-release-r1.xml) |
| ICU＋frontier Release | 34 PASS，9明确能力SKIP，0 FAIL | [构建](validation/final-icu-release-build-r1.json)、[逐项XML](validation/final-icu-release-r1.xml) |
| ICU＋frontier Debug | 34 PASS，9明确能力SKIP，0 FAIL | [构建](validation/final-icu-debug-build-r1.json)、[逐项XML](validation/final-icu-debug-r1.xml) |
| 18个单元独立Release构建 | 18个叶级配置/构建/测试完成；合计34 PASS＋9能力SKIP，0 FAIL | [逐叶汇总](validation/final-leaf-summary-r1.json)，各`final-leaf-<unit>-configure/build/ctest-r1.json` |
| ICU越界缓存负例 | 伪装成同版本、但位于隔离前缀外的头文件被配置阶段拒绝 | [预期exit1且命中特定诊断](validation/final-icu-outside-prefix-rejected-r1.json) |

ASan选择reference/observation/capability等安全标签，不将其17项扩大成所有good/bad或ICU都接受了插桩。前沿的9项包括编码识别、charconv结果bool、to_string、runtime_format、path formatter、两项格式化语义以及位移/位排列；宏/头缺失或明确旧语义各自说明原因。OFF则不注册能力测试，属于DISABLED，不计入SKIP。

## 3. Student、检查器与数据独立性

7个实现型单元为L03、L06、L08、L13、L14、L15和P1。最终Reference OFF配置中没有Reference目标；实际清洁重建产生2929条include记录，codemodel和本地include递归审计PASS，见[完整trace](validation/final-student-trace-build-r1.json)、[隔离审计](validation/final-student-isolation-r1.json)。该工具证明所声明路径/目标和真实接线，不是防抄袭证明。

Student初态19项中12个非作业检查通过、7个占位实现真实失败；这7项恰好等于声明的Student集合，无额外失败。见[初态核对](validation/final-student-initial-state-r1.json)和[逐项XML](validation/final-student-r1.xml)。记录器按预期exit8判定这次负面实验成立，不能把它翻译成学生已完成。

Reference、独立good完成体和bad控制体使用同一checker实际调用被选实现。L07身份误用、L09忽略截断另有独立错误控制。L15 good不调用canonical codec；旧v1 reader只认识tags1—5，确实读取v2包并有意丢弃未知note。P1共享之前章节的低层能力，评分对象为真实配置/文件/解码组合，没有借共享接口绕过本题待实现操作。

黄金包为独立手写68/74字节，布局在[fixture说明](../exercises/fixtures/README.md)，并在codec实现前完成[独立预检](validation/golden-preflight.json)。随后checker比较真实编码/文件字节，不仅做encode/decode自往返。

U01两种最终配置分别运行**19965条NormalizationTest记录、1093条GraphemeBreakTest记录**；各条按官方关系验证规范化四形式及字素边界，并核对UTF16/UTF8坐标。数据、来源、字节数、许可证和SHA保存在[Unicode16.0数据清单](../exercises/U01_icu_unicode/data/unicode-16.0-data-manifest.json)。它不证明所有Unicode操作或所有版本都已验证。

## 4. 发现、修复与复验闭环

| 问题 | 原因及修复 | 独立或最终复验证据 |
|---|---|---|
| 样章正文与bad机制不一致 | 初期控制体改成漏UTF校验，而正文仍以提前提交cursor推导；修为保留安全边界/UTF、仅故意提前提交 | [教学r1](reviews/sample-teaching-20260909-232127.md) → [教学r2](reviews/sample-teaching-r2-20260909-232708.md)、[技术r2](reviews/sample-technical-verifier-r2.md) |
| schema缺路径头时条件绕过 | 开发中`__has_include`造成局部检查缺路径语义；删除绕过，canonical/good/v1 reader强依赖唯一path validator，补wire全局offset和路径反例 | [数据独立核验](reviews/data-integration-verifier-r1.md)；最终全矩阵/叶级均使用必需依赖 |
| 配置换行/坐标与时间显示边界 | CR处理分支遗漏、NUL/UTF行号缺失、locale依赖、历史offset秒丢失及local年界限；先加复现再修，good/bad同步语法 | [修前复现](validation/time-path-author-r2-repro-20260909.md)、[作者复验](validation/time-path-author-r2-verification-20260909.md)、[非作者核验](reviews/text-time-technical-review-20260910.md) |
| P1临时父路径尾分隔符 | Temp带尾分隔符而child.parent_path不带，比较假不一致导致拒绝清理；规范化父路径，清理失败优先于预期bad诊断 | [实际复现](validation/p1-temp-repro-run-r1.json)、[原有4个自有目录清理](validation/p1-owned-temp-cleanup-r1.json)、[数据核验](reviews/data-integration-verifier-r1.md) |
| P1空清单绕过zone验证 | 没有记录便不进入格式化循环；对空清单显式验证zone后才创建文件 | [修前复现](validation/p1-empty-zone-repro-run-r1.json)、[修后P1复验](validation/p1-empty-zone-fix-ctest-r1.json)及最终两配置 |

核心和ICU当前没有已知未关闭技术阻断。样章门通过后才批量展开后续主题；修复后的旧发现没有简单改名为PASS，r1与r2记录分别保留。

## 5. 非作者审查与工具边界

- 教学：样章教学r2与[全课教学r4](reviews/course-teaching-r4-20260910-000258.md)批准；r4继承已审未变基础章节，新审其余章节/Part。其LOW交叉引用已改指全局性能规则，由最终交付核验检查该小改动。
- 数据/序列化/P1/公共构建：[独立verifier](reviews/data-integration-verifier-r1.md)APPROVE，另做duplicate ID wire、旧读者和资源路径不被打开等实际probe；样章Unicode/bytes oracle在未变范围内继承。
- 文本/时间/配置/ICU/前沿：[独立技术审查](reviews/text-time-technical-review-20260910.md)没有问题或阻断，实跑相关叶级、ICU两配置和契约probe。该角色因当前工具面没有LSP/AST工具将formal recommendation写为COMMENT；本交付采用实际MSVC编译、CTest、独立probe和逐代码检查作为替代，不伪称调用过缺失工具或将COMMENT篡改成其formal APPROVE。
- 最终核验单独检查矩阵/源码绑定、导航、交付清单、未测边界与本报告的真实性，不由作者批准自己的汇总。

## 6. 证据保存与剩余边界

最终结论只引用`final-*`及指定独立审查。早期ICU准备有失败/过渡记录和ignored半成品目录，最终依赖只使用`icu-prepare-20260909-231724`确认的sparse来源。前沿作者修正OFF状态时覆盖了部分早期成功探针记录并删除旧OFF测试记录；这些历史记录不再视为可复现证据，本次保留该限制，最终fresh OFF/ON语义和完整矩阵另有独立证据。没有用缺失记录支撑性能结论或现有通过数。

构建产物、第三方源码/库、临时probe在ignored build目录，不当作随仓库可访问文件；可发布内容是正文、源码、固定Unicode测试数据、构建入口与原始结果。逐文件交付和指纹见[交付清单](delivery-manifest.md)。

本轮未运行Linux/macOS，也没有安装新编译器补齐前沿实现。ICU准备脚本与已验证前缀针对Windows；其他平台保留通用核心命令、课程代码、实验输入和限制说明。ASan有限检查不证明所有生命周期；同步文件往返不保证事务、断电持久化或恶意并发修改安全；配置/路径词法规则也不冒充完整文件系统沙箱。这些边界不以SKIP或一次PASS抹去。
