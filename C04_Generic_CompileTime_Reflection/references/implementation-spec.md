# C04 实施规格

2026-09-10 新增授权及进度见[修订实施记录](revision-plan-20260910.md)。新增范围包括成熟库和C05跨课实验；以下原规格及已勾选项目保留为前次基线，不能代替本次新增范围的验证。

2026-09-09。用户已批准完整方案并要求实施。输入 HEAD `f261bea`，初始工作树干净。目标是完成泛型、编译期编程与反射课程，按根全局计划 4.2 的逐知识点下限及通用重构指引验收。方案已由非作者结构审查和 critic 审查批准；预设模型不可用时使用可用原生角色，不冒称完整 OMX ralplan runtime 门已执行。

## 已确认范围

新增本课正文、练习、答案、源码导读、构建及证据。修改根 README、全局计划 7.2、通用指南的组织原则，以及 C01/C02/C03/C06/C09/C10/C14/C15 README 桥接。旧正文、算法、学习文件、公共工具不改。无新依赖、CI、安装、机器配置、commit/push、凭据或生产系统操作。外部仅只读一手资料。

用户明确：组织依据教学与实际业务需求，LearnCPP 独立，内部合理耦合不是主要问题；此原则已纳入通用指南。选择机制专题递进与综合项目贯通；不把课程拆成彼此割裂的 C04A/C04B，也不强迫所有机制塞进同一业务类。允许完整 checkout 中复用已有工具。

## 读者、知识与单元

读者具 C01 最小构建及 C02 对象/存储/RAII 基础。C03 错误通道按实际用到的章节先修；C06/C09/C10/C14/C15 是后续应用，不作全课硬先修。

| 编辑位置 | 主讲及练习 |
|---|---|
| 00 | 阅读路线、诊断方法、证据分层与入口自测 |
| 01 / L01_templates | 声明、函数/类/变量/别名模板、实例化、特化、ODR及显式实例化 |
| 02 / L02_deduction | auto/decltype、cv/ref推导、数组函数退化、非推导上下文 |
| 03 / L03_forwarding | 引用折叠、转发、CTAD、初始化及可调用转发 |
| 04 / L04_lookup | 依赖名、两阶段查找、typename/template、ADL及hidden friend |
| 05 / L05_overload | 候选集、转换、函数偏序、偏特化、SFINAE与硬错误 |
| 06 / L06_constraints | requires、Concepts、原子约束/归一化/包含关系、偏序与语义公理 |
| 07 / L07_packs | 参数包、折叠、NTTP、模板模板参数、结构化字符串模板参数 |
| 08 / L08_type_lists | traits、type_list计算、类型集合/签名变换及诊断 |
| 09 / L09_tuple | tuple/index_sequence、异构遍历、cvref和求值顺序 |
| 10 / L10_constexpr | constexpr/consteval/if consteval、存储边界、编译期算法与容器 |
| 11 / L11_customization | invoke、CPO、表达式约束/返回类型/noexcept一致；复杂样章 |
| 12 / B01_compile_cost | 编译诊断、类型计算及多TU实例化成本、代码体积 |
| 13—15 / F01_frontier | 反射查询/splicing/annotations/展开/静态生成；C26/C29/提案分层 |
| 16 / P1_static_record | 字段访问、格式化和结构化字段序列化综合项目 |
| 17 | 固定源码导读与各下游课程回访 |

位置不是篇幅或质量上限。每个复杂机制需要背景、规则、推导、正反例、运行/编译入口和答案，不用外链代替主讲。演进在原问题复现/定位后才进入依赖它的下一步。没有收益也是有效结果。

## 公共接口及边界

样章 `c04::read_value`：合法 member 优先，否则合法 ADL；无路径约束拒绝；返回类型、cvref、noexcept依据实际表达式。已知类型的正确基线与更宽支持域的扩展分别标明，不用错误实现充当正确基线。

P1：C++23使用手工tuple/member-pointer描述；C++26另用真实反射，在相同支持域复验。`visit_fields(T&&, F&&)`按声明顺序传名称和保留cvref的成员；callback作为左值调用，不保存借用；空记录零调用，异常停止后续字段但不回滚外部副作用。

`encode_fields(const T&)`返回拥有名称/值的字符串字段列表。`decode_fields<T>(span<...>)`返回`expected<T, field_error>`，支持输入乱序，拒绝重复、未知、缺失和非法值；局部构造结果，分配异常传播。公开普通成员聚合的int/bool/string是codec支持域；int全字符十进制解析及范围检查，bool仅true/false，string原样。名称稳定性不自动保证schema兼容。annotations的字段排除只进入格式化示例，codec全部字段往返。

## 构建、工具与验证

C++23核心；CMake脚本3.28，VS2026预设4.2以上。实际本机CMake4.2.3、MSVC19.51.36256/工具集14.51.36231、STL202604、clang-cl22.1.3、Python3.10.11。MSVC include中没有meta；GCC13.2不作为反射实现。固定标准N4950、N5050和N5054/5055，适用DR另记；规范与实际能力分开。

复用C01 `check.hpp`和`process_runner.py`、C02 `record_process.py`与`audit_student.py`。C03的课程局部CMake模式用于本课，API为`c04_configure_target`、`c04_add_test`、`c04_add_observation`、`c04_add_exercise`。GENERIC_STUDY_BUILD_REFERENCE=ON、TEST_STUDENTS=OFF、ENABLE_FRONTIER=OFF、ENABLE_ASAN=OFF；标准预设verify-core/verify-debug/student/asan/frontier。

Student、Reference、good、bad独立；checker消费被选实现。Student可实例化占位与concept守卫允许构建，初始结果明确失败，不通过完成标记绕过。实现完成体通过，坏变体按具体checker诊断和exit1拒绝。负向编译隔离，先证明正例工具链正常，再匹配错误阶段/类别；环境错误、超时和清理失败不算预期拒绝。

核心Debug/Release、所有叶级、Student-only接线、相关安全路径ASan、frontier单项探测。OFF、能力SKIP、PASS、FAIL分别报告；已具能力但编译/运行错误仍FAIL。用户接受本机未支持前沿的完整文档/代码/实验说明及显式未测，不接受traits冒充反射。普通运行30s、单编译180s、整课configure180s/build900s外部上限。

成本实验：32/128/256项递归vs折叠查询；4TU隐式vs显式实例化。正确性后先trace和符号/section大小归因，再正式1次预热+5次独立进程采样，固定种子安排顺序。插桩与正式计时分组，测量时暂停其他build。全部原始结果、命令、编译/链接选项、环境和源码/二进制指纹保留。Python3.10驱动复用runner，不照搬硬编码且需file_digest3.11的C01 G2脚本。

## 所有权、进度与门槛

实施期间发现另一会话新增`C05_Data_Representation_Standard_Facilities/`。该目录及其其他改动不属于本任务；共享导航只增量合并C04段落，不回退并行工作。正式成本采样须观察其他编译进程，不能把已知并发构建混入有效比较。

主负责人独占公共接口/CMake、规格、覆盖、导航、P1、成本实验、集成和最终报告。样章作者独占chapter11与L11；样章自检后，非作者教学/技术审查及修复复验通过，才放行其他批量作者。为缩短独立主题的交付等待，后续作者分别负责01—03/L01—03、04—06/L04—06、07—10/L07—10、13—15/F01；成本作者另负责12/B01，正式采样时其他作者暂停构建；不可覆盖样章或公共文件。需要共享改动向leader报告。

审查用实际文件指纹绑定；作者修复后非作者复验。最终执行跨单元集成和遮答案教学检查，不能以作者完成或测试总量代替验收。

- [x] 方案获批；初始状态与工具链核对；组织原则纳入约束。
- [x] CPO样章完整且非作者批准：r1两项阻断经r2独立复验关闭，见reviews/sample-review-r2.md。
- [x] 全部主题、练习、解析、前沿说明和源码导读完成。
- [x] P1与成本实验完整证据。
- [x] 核心/叶级/Student/ASan/frontier验证与导航检查。
- [x] 非作者教学、技术、实验审查及复验阻断归零；集成终审见reviews/integration-review-final.md。
- [x] 最终质量报告、覆盖和全局状态一致；仅本课与约定导航/指南变更，并行C05保留。

完成仅指C04教材和本机可用路径；不宣称所有前沿/所有平台或18课全部完成。本机二进制/build不属于随仓库交付材料。无任何发布授权。
