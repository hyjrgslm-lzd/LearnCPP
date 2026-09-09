# C03 质量与交付报告

日期：2026-09-09。C03教材、实现、约定Windows本机验证及非作者独立终审均已完成。初始版本8e0f407afd155ede81f0b06a51553dadef206a23，初始工作树干净；新增C03尚未提交，实际交付版本以文件清单中的SHA绑定，不把初始HEAD当作已包含C03的提交。本次没有commit/push、安装或机器全局配置修改。

## 1. 实际交付

18章连续正文（00—17）；15个专题单元L01—L15、Document综合项目P1及F01前沿单元。包含8个实现型Student、独立Reference/good、按失效点隔离的bad，以及8个观察单元。每个必做Part都有正文或练习README解析。

覆盖类不变量、值/身份、optional/variant/expected、异常安全、参数/借用、组合继承、动态/静态多态、类型擦除、可调用包装、间接值、契约/hardening、接口演进、固定源码导读与真实下游回访。C++26/29内容保留完整讲解和真实源码，能力与运行结果单独登记；量纲桥接C13，模式匹配保持提案跟踪。

逐项入口见[覆盖表](coverage.md)，接口与边界见[实施规格](implementation-spec.md)，文件/指纹见[交付清单](delivery-manifest.md)。本课是新增内容，未搬走或删除旧课知识；根导航/全局计划和C02/C06/C09/C10 README仅增加衔接。

## 2. 实测环境

Windows/x64；CMake 4.2.3；Visual Studio 18 2026生成器；MSVC 19.51.36256.0，工具集目录14.51.36231；MSVC STL v145/update 202604。核心为C++23，frontier目标显式使用/std:c++latest。实际编译/链接选项与二进制指纹纳入交付清单。

课程CMake脚本最低3.28，但VS2026生成器在4.2才加入，故Windows预设要求4.2+。此说明依据本机官方Help/generator/Visual Studio 18 2026.rst修正；实际构建一直使用4.2.3，未因纯版本声明修正重复编译。

MSVC配置包含/utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus。ASan为RelWithDebInfo，绑定当前cl目录的匹配DLL。普通Debug保留默认迭代器诊断；P1分配注入有两个独立IDL0目标，原因见第5节。无新依赖或测试框架。

## 3. 全课集成矩阵

以下是全部单元到齐后的集成运行，不是将旧作者日志相加。过程JSON保存command、cwd、exit、timeout、stdout/stderr和清理结果；JUnit逐例分类。

| 配置 | 实际结果 | 证据 |
|---|---|---|
| Release核心 | **40 PASS / 0 FAIL / 0 SKIP** | [过程](validation/integration/ctest-release-r1.json)、[JUnit](validation/integration/ctest-release-r1.xml) |
| Debug核心 | **40 PASS / 0 FAIL / 0 SKIP** | [过程](validation/integration/ctest-debug-r1.json)、[JUnit](validation/integration/ctest-debug-r1.xml) |
| ASan安全路径 | **18 PASS / 0 FAIL / 0 SKIP** | [过程](validation/integration/ctest-asan-r1.json)、[JUnit](validation/integration/ctest-asan-r1.xml) |
| Student/ref关闭 | 8观察PASS；**8未完成Student按预期check失败**，无其他失败/跳过 | [过程，CTest exit8为预期](validation/integration/ctest-student-r1.json)、[逐项JUnit](validation/integration/ctest-student-r1.xml) |
| Student接线 | **PASS**，8目标、2409条实际include，无Reference目标/include/link依赖 | [接线审计](validation/integration/student-isolation-r1.json)、[clean-first trace](validation/integration/student-include-trace-r1.json) |
| Frontier全课 | **45 PASS / 8能力SKIP / 0 FAIL** | [过程](validation/integration/ctest-frontier-r1.json)、[JUnit](validation/integration/ctest-frontier-r1.xml) |

核心40项包含8个Reference、8个good、14个针对性negative、8个观察和2个allocation实验。Student初始失败不代表学生完成；各失败均有真实checker消息，没有用超时、缺DLL或完成标记冒充拒绝。

F01自身13项：3个C++23设施PASS、2个信息观察PASS、8个能力SKIP。Frontier的45项PASS还包含核心40项。跳过项为optional引用、optional range、variant成员visit、copyable_function、function_ref、indirect/polymorphic、C26 contracts、C29虚函数契约，原因见[能力记录](validation/capabilities/capability-results-20260909.md)。宏打印/提案信息不等于设施实现验证。

各单元另有独立叶级配置/构建/运行记录，入口见下方作者与审查证据。全课configure/build原始过程保存在integration目录。Student trace只在该次构建追加/showIncludes并使用VSLANG=1033，未改全局环境；审计不宣称能识别任意抄写算法。

## 4. 非作者审查与闭环

| 范围 | 已关闭问题 | 批准记录 |
|---|---|---|
| L06样章 | 不完整fixture、可抛提交、good同算法、固定复制次数、两个bad先撞同一断言；以共享fixture、独立good、行为检查和隔离失败面修复 | [教学](validation/reviews/sample-teaching-r4.md)、[技术r5](validation/reviews/sample-technical-r5-approval.md) |
| 00/01/02/07及L01/L02/L07 | 临时view最初仅验证Reference；增加当前实现requires检查与独立bad_rvalue | [基础切片](validation/reviews/foundations-author-a-r2-approval.md) |
| 03—05及L03—L05 | expected引用T错误表述、checked observer与前提混淆；正文、reference_wrapper正例和编译反例复验 | [状态切片](validation/reviews/states-independent-r2-approval.md) |
| 08—11及对应练习 | clone未检查值、擦除遗漏over-aligned目标；补值错误bad与alignas64检查 | [多态切片](validation/reviews/polymorphism-r2-approval.md) |
| 12—15与F01/索引 | callable ref/noexcept缺实验，C29虚契约缺组合规则；补真实标准调用及P3097推导 | [包装与契约](validation/reviews/callables-contracts-r2-approval.md) |
| P1与第16章 | 全局注入误伤Debug库noexcept代理分配；最小复现后分离实验，保留正常Debug | [P1独立终审](validation/reviews/document-p1-review-r1/document-p1-final-review.md) |

审查者未修改被审实现。修复使用新记录复验，旧失败/旧指纹仍保留，不把旧记录改成新版本批准。LSP/ast-grep工具未提供，本轮采用真实MSVC /W4构建、源码核对及编译/运行正反例作为可用诊断；没有声称运行不存在的工具。

**最终集成审查：APPROVE，已知阻断为0。** [独立终审记录](validation/reviews/final-integration-review.md)核对了第17章、跨模块导航、覆盖与证据，并验证候选清单514份文件的SHA全部匹配。批准后只回填本状态与全局进度，重新生成最终文件清单、核对指纹与导航；课程代码和已通过的运行结果没有改变。

## 5. 用户反馈的Debug abort

用户报告P1_document_validation_good.exe出现abort() has been called。初版检查器全局替换new，在事务动态调用中逐点抛bad_alloc，误伤MSVC Debug string移动构造内部的16字节代理分配。移动为noexcept，因而终止。最小string移动程序在默认Debug复现、独立IDL0对照成功；源码指向代理分配，不是vector::swap。

修复后普通P1目标不替换new、保留Debug迭代器检查；两个命名allocation目标继续对同一Reference/good源码逐实际分配点验证，仅它们定义IDL0。测试进程局部配置CRT/terminate输出，意外终止仍非零失败但不弹窗口，未改机器设置。

原报错路径已重新构建并运行通过。Debug/ASan叶级各5/5，非作者又复跑Debug/Release/ASan各5/5。完整复现、归因和边界见[诊断记录](validation/p1-debug-allocation/diagnosis.md)。

## 6. 导航、保护与可携带性

导航检查只证明目标/锚点存在，教学深度另经非作者审查。[首轮导航](validation/integration/navigation-r1.json)核对76文档、245处本地链接，无断链；最终报告/清单完成后再检查登记。

[共享工具保护](validation/integration/scope-and-helpers-r1.json)确认四个C01/C02复用工具SHA未变，旧课源码未改。根导航两个文件的更新也在批准范围。没有暂存、commit/push；build、exe、PDB与缓存不属于交付文件。

原始.log被仓库忽略的，已保存同名字节相同.txt供交付，原.log仍在本机。L07一组证据曾误写根references，已按[迁移清单](validation/evidence-relocation-r1.json)保持字节归位，历史审查原路径按此映射定位。三份JUnit初始由CTest在binary相对目录输出，按[拷贝记录](validation/integration/junit-relocation-r1.json)保存到可跟踪位置，没有改写内容。

指纹绑定本次源码/文档/数据与实际本机二进制。其他机器不要求生成同一二进制；Git换行变化可同时对照文本归一化指纹。

## 7. 明确限制

- 只验证Windows/MSVC，其他平台/标准库未测；提供配置方式不等于实测通过。
- 八个C++26/29能力测试未运行受保护的新语法分支，未验证contracts checking mode/violation。完整源码和前提保留。
- 安全借用模型、ABI布局风险模型和宏观察仅证明各自范围；没有声称实际悬垂访问、跨编译器ABI兼容或一般OOM恢复。
- 分配/复制计数不是性能排名，本次无时间加速结论。
- C03完成不等于学生完成8道实现题，也不代表G1或18课全局完成。
