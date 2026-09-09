# C02 质量与验证报告

> 2026-09-09目录与顶层项目名已迁移；本报告原验证结论及SHA绑定当时版本。新路径验证、项目命名与历史记录格式边界见[目录迁移记录](../../C02_Objects_Lifetime_Ownership/references/directory-migration.md)。

规范基线：2026-09-08。最终本机验证：2026-09-09。状态：C02 Windows范围实施、验证与非作者最终整合审查完成。

## 交付范围

16个正文单元（00—15）、15个练习单元（L01—L14/P1）。正文、Student、Reference、观察程序、公开好坏变体及解析按[覆盖表](coverage.md)相连。P1保持只读span、move-only容器、规定类型前提和强异常保证；L10保持单线程教学模型。根导航及五课README仅补先修桥接，旧课源码与用户学习代码未修改。

逐文件清单见[delivery-manifest](delivery-manifest.md)。验证中的编译产物由全局及本课.gitignore排除；公开原始JSON/TXT/XML和诊断源码保留。此次没有提交、推送、安装或全局机器配置修改。

## 最终本机矩阵

| 验证面 | 实际结果 | 证据 |
|---|---|---|
| MSVC Debug | 40/40通过，0跳过 | [最终CTest](validation/integration/verify-debug-ctest-final-r2.json) |
| MSVC Release | 40/40通过，0跳过 | [最终CTest](validation/integration/verify-core-ctest-final-r2.json) |
| Student隔离 | 9个目标构建成功；9个未完成起点均被真实检查拒绝；无Reference依赖 | [实际include/源码/目标审计](validation/integration/student-wiring-final-r1.json)、[预期失败](validation/integration/student-initial-final-r1.json) |
| Clang++ ASan安全路径 | 20项中19通过、1能力跳过；0失败 | [原始记录](validation/integration/final-asan-r2.json)、[JUnit](validation/integration/final-asan-safe-r2.xml) |
| 显式unsafe ASan | 独立1项通过：匹配heap-use-after-free、源码位置和非零退出 | [JUnit](validation/integration/final-asan-unsafe-r2.xml) |
| MSVC前沿配置 | 普通build成功；45项中42通过、3能力跳过 | [整课原始记录](validation/integration/frontier-ctest-final-r1.json) |
| 收紧后的前沿诊断/fixture | 5项中2通过、3能力跳过；baseline先行 | [最终定点复验](validation/integration/frontier-probes-final-r3.json) |
| ClangCL ASan修复 | L12/P1/L14原失败目标重新build/run通过，DLL指纹匹配 | [非作者复验](validation/reviews/verifier-storage-11-15-final-r2.md) |
| 记录器与接线工具 | 7个记录契约通过；真实好坏接线及缺输入/错配置负控通过 | [记录器自检](validation/tools/self-check-dialog-mode-r2/summary.json)、[接线独立审查](validation/reviews/verifier-student-wiring-r1.md) |

Student初始失败是作业起点的预期状态，不计为Reference失败，也不伪装成学生已完成。各叶级独立配置、Debug/Release、Student/ref-off和公开good/bad的fresh验证已在分批独立报告中执行；总测试数量不能代替不同验收面的含义。

### 能力跳过与未验证范围

Clang22.1.3配当前MSVC STL未提供`__cpp_lib_start_lifetime_as`，因此L12对应能力测试跳过；同一能力在MSVC实际实例化、链接、运行通过。MSVC前沿P2287、P2748、P2953分别按选定模式的真实诊断/接受情况跳过。provenance示例仅证明compile-only模型可编译，不宣称已运行验证全部DR语义。

前沿测试默认不参与ALL构建，使用独立编译、baseline fixture、资源锁、具体诊断和新输出文件；缺目标、缺源码、超时等基础失败不能冒充能力缺失或正确拒绝。规范位置、实现状态及来源见[标准索引](standards-and-implementations.md)。Windows之外未实测，不外推为其他平台通过。

## 独立审查与修复闭环

| 切片 | 最终非作者依据 |
|---|---|
| 00/04与07样章 | [教学/代码](validation/reviews/reviewer-c02-l07-r3-final.md)、[技术/实验](validation/reviews/verifier-l07-r3.md) |
| 01/02/03/05/06 | [教学/代码](validation/reviews/reviewer-foundation-01-06-r1.md)、[技术/实验](validation/reviews/verifier-foundation-r1.md) |
| 08/09/10 | [原接口问题及L10批准](validation/reviews/reviewer-owner-08-10-r2.md)、[最终文档关闭](validation/reviews/reviewer-owner-08-10-r2-doc-interface-closeout.md)、[技术/实验](validation/reviews/verifier-08-10-r2.md) |
| 11—15 | [教学/代码最终关闭](validation/reviews/reviewer-storage-11-15-r2-final.md)、[技术/实验最终关闭](validation/reviews/verifier-storage-11-15-final-r2.md) |

已关闭主要问题：学生回填报告或完成标记、受信fixture被同名头遮蔽、移动赋值释放顺序、L08/L09说明与接口漂移、L10从被管理对象内部成员赋值的UAF、L12负例诊断参数被拆开，以及前沿测试宽松失败判定。原始失败与针对性复现保留，修复经非作者复验，不能用作者自测替代。

### 两类ASan问题分别记录

1. **L14启动弹窗：** MSVC v145程序误解析LLVM22同名DLL。二者导出不同，后者缺少`__sanitizer_cov_8bit_counters_cleanup__dll`。[静态PE证据](validation/debugger-p1-asan/runtime-mismatch-r1.md)确认这一点。构建现在按实际compiler定位匹配DLL并复制到输出目录；ClangCL采用对应import/thunk/SEH链接，验证工具只对子进程设置错误模式。用户报错的同一程序已[重跑退出0](validation/integration/l14-msvc-runtime-corrected-r1.json)。
2. **P1早期异常重抛：** 在当时工具链组合的原构建中，独立rethrow最小例和P1相应路径出现ASan/EH失败；普通throw及无ASan对照通过。原C++写法语义合法，具体更底层组件原因未从这些观察唯一确定。P1采用作用域rollback后完整ASan检查通过，未禁用插桩或删除失败注入。[诊断补充与时间线勘误](validation/debugger-p1-asan/runtime-mismatch-p1-clangpp-r3.md)明确：后续旧obj相对新RAII源码过期，不能反推最初失败只因stale artifact；也没有证据把它与L14 DLL混配合并。

## 最终整合签署

[教学与文档整合](validation/reviews/reviewer-integration-final-r1.md)、[技术与证据整合](validation/reviews/verifier-integration-final-r1.md)均为APPROVE。候选299个源文件与1266个证据文件由两位非作者独立重算，缺失、重复和指纹不匹配均为0；签署后的状态文档变更另以[最终快照](validation/snapshots/final-r1/summary.json)绑定。前沿各次编译原始输出见[归档索引](validation/integration/frontier-final-status-r3/index.txt)。

## 历史证据缺口与最终版本

L07作者曾错误清理早期中间JSON，包括CTest预期退出码误写为1的调试失败；[历史说明](validation/author-owner-l07-evidence-history-note.md)逐项登记无法无损恢复的记录。没有伪造恢复，也没有用重跑冒充旧记录。各版final及非作者原始记录仍保留；后续记录全部使用新前缀。

最终源文件与公开证据分别生成SHA256清单；清单与review报告自身排除循环自哈希。主线测试与后续受影响路径复验共同对应最终实现，初步preflight单独保留，不作为最终通过替身。本报告最终签署只覆盖C02，不代表18门课程全局完成。
