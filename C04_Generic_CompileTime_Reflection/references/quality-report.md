# C04 质量报告

交付日期：2026-09-10。范围为C04教材与约定Windows可用路径，包含18章、14个练习/实验单元、12个独立实现型Student，及正文、解析、源码导读、实际验证和成本实验。[覆盖表](coverage.md)按全局计划4.2逐项反查；文件数量不是质量指标。

2026-09-10修订增量入口：[revision-quality-report-20260910.md](revision-quality-report-20260910.md)。旧统计保留为历史基线；新增18—24章、A01—A05、U01/U02、依赖准备、L07—L10修复和前沿补充的当前状态以增量报告为准，等待root最终矩阵后再写入最终总数。

初始HEAD为`f261bea559d6722c31135fb2d3589be52fe958ed`，初始工作树干净。本次没有commit/push；实际交付字节以[逐文件清单与冻结指纹](delivery-manifest.md)为准。实施期间出现C05并行工作，未纳入C04交付或回退其修改。共享导航保留双方增量，其整文件哈希不表示C04独占创作。

## 实际交付与知识归属

01—06主讲模板声明/实例化、推导、转发、查找、重载、SFINAE与约束；07—10展开包、类型算法、tuple与常量求值；11完成CPO样章；12包含真实编译成本；13—15完整解释反射/生成/annotations及前沿版本；16完成字段项目；00/17提供阅读与真实源码/下游回访。

保留旧课正文、算法和学习文件，只增加约定的C01/C02/C03/C06/C09/C10/C14/C15 README桥接、根导航和全局进度。通用指南增加用户指定的组织原则：按教学/业务需求组织，允许合理内部耦合，不为解耦增加无关结构。共享工具只读复用C01检查器/进程运行器、C02记录器/Student审计器和C03导航解析函数。

P1在同一支持域下提供手工metadata和真实反射后端，二者共享codec与契约检查器。手工schema完整性是注册责任，专门的遗漏观察说明边界；反射主体本机未测，不能用手工路径通过代替。C05/C12的完整协议/schema兼容和C15的UHT/GC仍由对应课程主讲。

## 环境与实际选项

Windows/x64；CMake4.2.3；Visual Studio18 2026生成器；MSVC19.51.36256.0、工具集14.51.36231、STL更新202604；Python3.10.11。核心源码要求C++23，当前CMake/MSVC实际生成`/std:c++latest`，不能把最低语言要求误写成实际命令。各配置的真实编译/链接命令与二进制指纹随清单记录。

MSVC使用`/utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus`。ASan是RelWithDebInfo，绑定当前cl目录的匹配runtime DLL。没有安装组件、改变机器全局配置、增加依赖或CI。clang-cl22.1.3及LLVM工具只用于本机成本实验，其版本、flags和哈希在实验raw中。

本机没有`<meta>`，前沿能力按每个probe分别记录；固定规范、DR与提案边界见[规范与实现索引](standards-and-implementations.md)。LSP工具不可用，采用真实MSVC构建/诊断、原始结果和非作者源码核对，没有声称运行过不存在的工具。

## 验证矩阵

| 路径 | 实际结果 | 可复现证据 |
|---|---|---|
| root Debug | 59 PASS，0 FAIL，0 SKIP | [JUnit](validation/root-debug-01.junit.xml)、[构建](validation/root-debug-build-01.json)、[CTest](validation/root-debug-ctest-01.json) |
| root Release | 59 PASS，0 FAIL，0 SKIP | [JUnit](validation/root-release-01.junit.xml)、[构建](validation/root-release-build-01.json)、[CTest](validation/root-release-ctest-01.json) |
| root ASan安全路径 | 20 PASS，0 FAIL，0 SKIP | [JUnit](validation/root-asan-01.junit.xml)、[构建](validation/root-asan-build-01.json)、[CTest](validation/root-asan-ctest-01.json) |
| root Student-only | 12个初始Student真实失败；其余22项PASS；无其他失败 | [JUnit](validation/root-student-01.junit.xml)、[分类判定](validation/root-student-verdict-01.json)、[运行](validation/root-student-ctest-01.json) |
| Student独立接线 | 12个目标，1,794条实际include记录，0个违规依赖 | [新鲜include-trace](validation/root-student-include-trace-02.json)、[审计](validation/root-student-audit-02.json) |
| root frontier | 59 PASS、13 SKIP、0 FAIL；13项主体未运行 | [JUnit](validation/root-frontier-01.junit.xml)、[构建](validation/root-frontier-build-01.json)、[CTest](validation/root-frontier-ctest-01.json) |
| 单题构建 | L01—L11、P1、B01、F01分别有实际leaf验证 | 下表各作者/非作者记录；[构建指南](BUILD_GUIDE.md)给统一复现方式 |

核心通过数包含Reference、观察、good/bad检查器控制及预期编译/链接拒绝，不等于59个学生作业完成。Student的exit8来自CTest汇总其初始占位失败，每个失败都命中题意相关checker，不能写成课程失败或学生已完成。

ASan预设只选L02/L03/L04/L09/L11/P1的reference、validation、observation。编译负例、故意拒绝控制不混入安全路径统计。F01 OFF是未注册测试，不计为通过；ON的12项和P1反射项各自因能力不足SKIP。受控声明能力后返回1的实验实际被CTest判为Failed，未被77吞掉。

诊断运行器先成功编译/链接control，再要求subject在指定语义阶段失败；[归档明细](validation/final-diagnostics/index.json)包含control/subject完整结果和输入SHA。[正反控制](validation/compile-runner-r2-good-detail.json)及[成功编译不应算拒绝](validation/compile-runner-r2-false-detail.json)验证运行器本身。超时、启动失败、基础设施错误及清理失败不能作正常拒绝。

## 实际成本实验

最终采用[004959摘要](benchmarks/results/compile-cost-run-20260910-004959/summary.md)、[raw](benchmarks/results/compile-cost-run-20260910-004959/raw.json)和[即时事件](benchmarks/results/compile-cost-run-20260910-004959/events.jsonl)。外层[运行记录](validation/cost-author/12-full-driver-after-clock-boundary.json)PASS，194.031秒；8组各1次成功预热、5个有效独立样本，共40个正式有效样本。534条事件保留命令、活动探测和阶段关系。

| 场景 | 版本 | 中位数（秒） | 范围（秒） |
|---|---|---:|---|
| 32项类型查询 | recursive / fold | 0.094 / 0.094 | 0.093—0.110 / 0.094—0.125 |
| 128项类型查询 | recursive / fold | 0.110 / 0.109 | 0.109—0.141 / 0.094—0.125 |
| 256项类型查询 | recursive / fold | 0.157 / 0.109 | 0.140—0.172 / 0.093—0.125 |
| 多TU编译 | implicit / explicit | 0.375 / 0.438 | 0.344—0.422 / 0.422—0.453 |

本机runner的单调时钟是`GetTickCount64()`，分辨率15.625ms；显示小数位不是测量精度。32/128项不能据此推断稳定收益，尤其不能把1ms差异解释为优化。多TU累计4/5个短进程时长，量化误差也需考虑；本文没有把它当完整构建耗时或保证每次的性能排名。

256项结合`Total Frontend`观察到差异；`Total InstantiateClass count`只是trace的Total scope计数，不等于所有嵌套实例化数量。显式实例化的调用方对象`.text`从336减到28字节，全部对象`.text`从705减到474字节，最终exe`.text`virtual均为90,861字节；未证明编译时间更快。编译器实例化/代码产生和链接合并是不同成本，不能用obj变小推出程序变小或变快。

两版多TU使用定义良好的uint32算术，逐个输出与独立reference核对并相互一致。trace与正式计时分组，main和link在多TU实验中仅承担正确性/section检查；implicit计4个调用方编译，explicit计4个调用方加provider。进程PID/CPU探测不能排除所有短暂任务或系统噪声，条件和限制均保留在数据及正文。

## 非作者审查与修复闭环

| 批次 | 发现与修复 | 当前范围判定 |
|---|---|---|
| CPO样章 | 补合法ADL fallback正例和改进前安全反例，保留初稿证据缺口 | [r2独立复验通过](reviews/sample-review-r2.md) |
| 01—06 | 修P2266/std::forward说明、多TU入口、特化检查、偏序/硬错误反例及subsumption归因 | [7项关闭，r2通过](reviews/foundations-review-r2.md) |
| 07—10 | 补类型算法推导/惰性反例、立即求值/存储主线和解析诊断，闭合包计数承诺 | [r2通过](reviews/metaprogramming-review-r2.md)；末尾接口名已统一为constexpr_vector_sum |
| P1 | 修缺能力分支误将__has_include放到运行时表达式 | [r2通过](reviews/record-review-r2.md)，真实反射主体未测 |
| 前沿 | 修meta vector存储、坏subject决定SKIP、P4101/P3385 API/标记和门控一致性 | [r2复验](reviews/frontier-review-r2.md)、[已确认交付范围通过](reviews/frontier-review-r2-delivery-contract.md) |
| 成本 | 修UB/弱检查、COFF解析、驻留节点识别、持久化、干扰/样本门及clock说明 | [最终r2数据与有限结论批准](reviews/cost-review-final-r2.md) |
| 集成 | 核对00/17、公共工具、接线、矩阵与导航；最终按字节清单核对 | [准备检查](reviews/integration-review.md)、[最终冻结复核](reviews/integration-review-final.md) |

作者与审查者分开。旧失败/旧源码指纹仍保留原义，不改写为新版本通过。早期成本启动等待尚未形成有效样本，之后采用逐命令事件持久化；09 full因warmup门错误失败，后续旧PASS亦保留但不作为当前正文的最终数据。当前有效结论只绑定004959和对应driver SHA。

Student首次trace的构建exit0，但记录器只匹配英文而判FAIL；新trace02重新构建并用同时支持中文/英文的独立审计器确认通过。编译负例工具的旧日志覆盖control字段、失败输出编码和Windows长路径问题均修复后重跑。未把工具/期望文本错误伪装成C++算法错误。

## 交付边界

这里只完成C04及约定导航，不宣称G1或18课全部完成。真实C++26/C++29前沿、其他平台/标准库未运行的部分明确保留；源码存在、语法支持表、能力SKIP和完整运行通过各有不同含义。参考实现/有限测试/ASan无报告也不证明所有输入和所有编译器行为。

构建产物只存在本机；随仓库的材料是源码、正文、文本/JSON/XML证据及冻结清单。清单记录本机二进制/编译命令元数据，但不把这些二进制包装成已交付附件。并行C05变动不归本批验证；导航检查将其共享入口另外列出，不擅自修它的内容。
