# C03 实施规格

日期：2026-09-09。用户已确认整套方案并要求实施。输入为根目录 CONTENT_REFACTORING_GUIDE.md、LEARNCPP_GLOBAL_PLAN.md，仓库初始版本 `8e0f407`，初始工作树干净。原生 Plan 阶段的设计审查及 critic 均批准；本文将获准行为落为执行规格，不以计划存在宣称课程完成。

## 目标、读者与范围

完整建设类型建模与接口设计课程。读者具基础 C++、C01 最小构建能力、C02 对象/生命周期/RAII 能力。正文负责连续推导，练习负责验证理解。当前需要的模板、泛型 lambda、约束和转发必须在使用前解释；尚未交付的 C04 不是隐含硬先修。

新增本课 chapters、exercises、references。额外只修改根 README、全局计划 7.2 和 C02/C06/C09/C10 的 README 导航。旧课正文、算法、学习文件、通用指南、共享工具保持原状。无新依赖、CI、安装、机器配置、commit/push、凭据、生产系统或破坏性操作。来源网页只读。采用可用原生 agents；不创建 Codex goal、不启动 tmux runtime。

## 组织决策

原则：完整知识覆盖；不变量与失败语义先行；实际证据先于结论；复用现有工具；非作者批准。

驱动：C03 概念可通过一个对象模型连接；下游需要真实的类型/错误/擦除机制；默认构建应离线可重复。

选择“主案例贯穿＋局部机制实验”。相比全课零散案例，它减少术语切换；相比把所有机制放进单一万能类，它保留方案成立的不同契约。最终 Document 使用封闭 variant，开放虚接口、静态多态、类型擦除、间接值各有完整独立分支。分支不是性能排名。新增场景或加强失败保证须重新声明比较口径。

## 课程与练习映射

| 章 | 主题 | 练习 | 深度与边界 |
|---|---|---|---|
| 00 | 模型与路线 | 自测，链接后续单元 | 先修、术语、值/身份/不变量、正文与实验的证据层次 |
| 01 | 不变量与强类型 | L01_invariants | 有效状态、封装、构造拒绝、显式转换；量纲类型桥接 C13，expected 工厂在05解释后回访 |
| 02 | 值语义 | L02_value_semantics | 复制独立性、身份、相等/排序/哈希、regular的语法满足与语义公理 |
| 03 | 可选状态 | L03_optional | 空值、访问、emplace/转换、monadic；C26 optional引用重绑定与0/1 range |
| 04 | 封闭状态集合 | L04_variant | 状态机、visit、重复类型/索引、valueless异常路径、C26成员visit |
| 05 | 错误通道 | L05_expected | expected/void、错误分类、monadic和抛异常边界、exception_ptr、工厂 |
| 06 | 异常安全与批事务 | L06_transactions | basic/strong/nothrow、失败注入、prepare/commit、保证边界；复杂样章 |
| 07 | 参数与返回接口 | L07_interfaces | 值/借用、const/ref限定、noexcept、避免暴露可写内部状态 |
| 08 | 组合与继承 | L08_composition | 组合、替换性、NVI、访问控制、继承与类型关系 |
| 09 | 动态多态 | L09_dynamic_polymorphism | virtual/override/final、析构、切片、RTTI、clone、多继承语义边界 |
| 10 | 静态多态 | L10_static_polymorphism | 当前所需模板/约束/CRTP完整推导，查找和实例化边界；深层泛型桥接C04 |
| 11 | 类型擦除 | L11_type_erasure | heap-only拥有型any_shape、操作表clone/destroy/dispatch、空态/移动/异常；固定实现源码对照SBO |
| 12 | 可调用包装 | L12_callables | invoke/reference_wrapper/function/move_only_function/copyable_function/function_ref，所有权、const/ref/noexcept、空调用 |
| 13 | 间接值语义 | L13_indirect_values | indirect/polymorphic，深复制、const传播、移动后状态、分配；与unique/shared不同责任 |
| 14 | 契约与库安全 | L14_contracts | 输入校验、断言、契约、hardening独立边界，C29虚函数契约，模式匹配提案跟踪 |
| 15 | 接口演进 | L15_evolution | 源/行为/ABI兼容、pimpl桥接C01、旧客户端正反例 |
| 16 | 文档模型集成 | P1_document | 所有下述公开契约、失败、快照和版本兼容 |
| 17 | 源码与下游回访 | 各单元证据入口 | 对应固定源码状态/入口/退出路径；C06/C09/C10实际调用反查 |

18章是编辑位置，不是教学质量指标。每个自测/Part需完整解析。源码导读穿插在相应章节，17章只汇总，不能用源码链接替代推导。

## 样章门与公共契约

先交06与00—05必要先修：符合basic保证的正确批更新基线；实验元素第k次复制失败，观察部分更新但无资源遗留；随后把需求加强到strong保证，以临时副本准备、nothrow swap提交；原失败点、空输入及成功路径重新检查。故意违反声称契约的程序是独立bad反例。受控throwing fixture证明该失败通道，不冒充实际物理内存耗尽。样章正文、Student、Reference、good/bad、解析、原始记录经过非作者审查及返修复验，才展开后续批量。样章通过不是整课完成。

最终 Document 单线程，在内存处理结构化图形，不做GUI/解析/网络。ElementId为非零强类型，尺寸为正值。Shape=variant<Rectangle,Circle>；Element绑定ID与Shape。vector保存插入顺序，ID唯一。值相等按ID、图形备选和字段；Document按有序元素序列比较，不按地址或分配器状态比较。复制独立。

- find(id)返回optional<Element>值快照，缺值不是错误；snapshot返回有序值集合。
- Edit=Add/Replace/Erase。apply(Edit)和apply_batch(span<const Edit>)返回expected<void,EditError>。
- Add重复ID拒绝；Replace/Erase缺失ID拒绝。Replace不改变位置，Erase保留其他元素顺序。
- batch按输入顺序解释，空批成功；先在临时文档准备，全部成功才以nothrow swap提交。失败不提交整个批。
- 校验错误经expected返回。复制/分配异常允许传播，同时保持调用前文档状态；不保证实参求值和外部副作用回滚。
- callback不存入copyable Document；同步借用和拥有move-only注册在单独接口实验验证。

any_shape固定最小heap-only操作表实现，不默认增加自定义SBO、allocator或插件框架。必须声明空态、复制/移动/自赋值、受约束目标、析构与失败保证。目标选择和分配计数是当前实现观察，不能推导所有STL的SBO阈值或运行性能优劣。

## 公共构建与工具

完整checkout内整课/单题构建。C++23，CMake脚本最低3.28；Windows的VS2026预设要求CMake4.2+（生成器在4.2加入）。项目名与目录一致；MSVC /utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus。使用本课最小exercises/cmake/StudySetup.cmake，公开函数c03_configure_target、c03_add_test、c03_add_observation、c03_add_exercise。

只读复用实际路径：

- `../../C01_Build_Compile_Link/exercises/include/check.hpp`（相对本references目录）。
- `../../C01_Build_Compile_Link/exercises/tools/process_runner.py`。
- `../../C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`。
- `../../C02_Objects_Lifetime_Ownership/exercises/tools/audit_student.py`。

相对路径用于定位，链接见BUILD_GUIDE。实现不得误建新common目录或改动共享helper。每题src/student与src/reference使用各自真正独立实现；完整输入类型与失败注入fixture可共享，不能让学生先补fixture才能实现题目承诺的操作。checker共享且实际包含被选实现；good完成体不通过include Reference偷渡答案。

选项TYPE_STUDY_BUILD_REFERENCE=ON、TYPE_STUDY_TEST_STUDENTS=OFF、TYPE_STUDY_ENABLE_ASAN=OFF、TYPE_STUDY_ENABLE_FRONTIER=OFF、TYPE_STUDY_ENABLE_UNSAFE_DEMOS=OFF。Presets：verify-core Release、verify-debug Debug、student(ref关闭)、asan安全RelWithDebInfo、frontier。观察型单独标签，不伪造学生作业；Student初始明确失败，Release的check不受NDEBUG影响。

frontier每设施独立源代码、独立头/宏/真实语法实例化/链接/运行记录。未启用与能力缺失不同，只有明确缺能力可SKIP；已满足能力条件后构建或运行失败是FAIL。C26/29不能替换命名空间模拟支持。最低标准与实际/std模式分开，固定规范和本机安装STL版本/头文件指纹不等于固定上游Git提交。

## 验收矩阵

1. 不变量无效输入/转换、值比较与独立复制；optional空/有/短路；variant合法访问/重复类型/可复现valueless条件；expected值/错/void/monadic回调抛异常。
2. basic与strong各自正确边界、每k步失败回滚/清理；动态clone和切片、虚析构；擦除复制/移动/自赋值/异常/空态/over-aligned目标；借用模型与真实悬垂证据分开。
3. Document重复/缺失ID、保序、空批、前一操作成功后一操作失败时整批回滚、快照/复制独立性。兼容实验保留v1客户，在v2头下检验源码可编译及行为，不把源码兼容当ABI保证。
4. 每实现题good通过、bad由预期checker诊断和精确退出码拒绝。无操作、恒值、别名复制、提前提交、吞错误等按风险选择。Student/ref关闭，实际预处理include及链接/target依赖审计；初始失败另记录，不混为课程FAIL。
5. fresh MSVC Debug/Release整课、所有叶级配置与验证、Student好坏及隔离、ASan安全路径、frontier逐项状态。普通测试外部超时30秒，构建/编译诊断受外部180秒或明确记录的有界超时覆盖。启动、缺DLL、超时、清理失败均FAIL，不当负例通过。
6. 链接、锚点、逐知识点覆盖和非作者遮答案教学审查；C06/C09/C10下游使用反查。旧课仅README改动时检查导航与非授权源码diff为空。

默认不作时间性能排名。复制/移动/分配计数只证明特定输入的执行路径。如实际提出性能比较，先取得可归因证据，再1次预热+5次独立进程采样，保存全部结果与代价。

## 作者、证据与停止

Leader独占公共接口、构建、规格、coverage、导航、质量报告和16—17。执行中为加强状态/错误正文，作者A负责00—02、06—07，独立状态作者负责03—05完整主题链，B负责08—11，C负责12—15；能力作者负责F01与标准索引。非作者分别审教学及技术/实验，受审版本冻结并绑定指纹，修改后对应结论需重新复验。

样章门按真实依赖审查：L06使用C02复制/RAII/vector/span及00—02不变量，不使用optional/variant/expected。03—05的修订仍是整课完成阻断，独立完成并另审；不把章节编号当作06的硬先修。样章技术与教学批准后只放行其余作者，全部18章的最终门槛保持不变。

最终保存命令、输入、环境、flags、exit/timeout、stdout/stderr、源码/数据指纹与审查闭环。约定正文/Part/答案/实现/实验全部交付，可用路径通过，已知阻断清零且非作者批准后才登记“C03教材与Windows可用路径交付完成”。未实现C26/29、其他平台未验证显式列出，不当PASS，不声明全部G1或18课完成。
