# C02 实施规格：语言核心、对象模型与资源管理

批准：2026-09-08，用户明确要求 Implement the plan。本文将会话中经 Planner、Architect、Critic 顺序审查通过的 v3 与最终获准计划落实为作者共同规格。计划批准不代表实现、实验或教学已经通过审查。

## 目标与边界

完整新增 C02_Objects_Lifetime_Ownership，面向已有基础 C++ 编程经验、尚未系统掌握专题的工程师。采用“分专题＋综合项目”：正文从必要背景和连续因果推导进入真实代码、实验及工程应用；练习验证理解。所有自测、实现 Part 和关键实验提供完整解析，不以术语表、外链或 Reference 代替正文。

遵循根目录 LEARNCPP_GLOBAL_PLAN.md 和 CONTENT_REFACTORING_GUIDE.md。原则是先修闭合，规范/实现/实测分层，检查有效，最小复用，非作者审查。选独立 C02_Objects_Lifetime_Ownership 而非扩入 Engineering：保留 C01 工具链与 C02 对象模型的清楚职责，代价以最小跨课导航承担。

范围：新增本课；根 README、全局计划回填 C02；Engineering、Coroutine、Concurrency、Execution、Ranges 五课仅改 README 先修链接。其他旧课正文、算法、学习代码与通用指引不改；独立缺陷记录在交接。没有新依赖、CI、系统安装、全局设置、commit/push、生产系统、凭据或破坏性操作。用 native agents，不创建 Codex goal、不启动 tmux runtime。

## 课程与目录责任

| 章 | 正文文件（chapters/） | 练习目录（exercises/） | 主讲与实现深度 |
|---|---|---|---|
| 00 | 00-model-and-route.md | — | 术语、能力自测、对象/资源模型、阅读与实验方法 |
| 01 | 01-initialization.md | L01_initialization | default/value/list/aggregate 初始化、静态/动态初始化、constinit、cv、转换/窄化、求值顺序；观察及编译正反 |
| 02 | 02-expressions-and-references.md | L02_value_categories | 值类别、引用绑定、auto/decltype、最低限度引用折叠与转发；观察及编译正反 |
| 03 | 03-lifetime-and-borrowing.md | L03_lifetimes | 对象/存储/存储期/生命期，临时延长、返回/成员/initializer_list/range-for、lambda/view；安全 owner 与借用练习 |
| 04 | 04-construction-and-unwinding.md | L04_construction | 成员/基类/委托构造与析构顺序、new/delete 初步、部分构造失败及清理 |
| 05 | 05-special-members.md | L05_special_members | copy/move、default/delete、生成/抑制、Rule0/5、深复制；前置不依赖尚未讲解的07 |
| 06 | 06-move-and-return.md | L06_move_return | std::move/const、prvalue 结果对象、必然消除/可选 NRVO、implicit move、noexcept/move_if_noexcept |
| 07 | 07-raii-and-ownership.md | L07_raii | RAII、资源转交、独占/共享/借用；两个资源的部分构造失败样章及最小 move-only owner |
| 08 | 08-unique-ownership.md | L08_unique | unique_ptr/deleter/数组/不完整类型/get/release/reset；标准设施正确使用及源码 |
| 09 | 09-shared-ownership.md | L09_shared | shared/weak/aliasing/cycle/enable_shared_from_this、计数同步不等于对象同步；生命周期图与循环修复 |
| 10 | 10-control-block.md | L10_control_block | 单线程 rc_ptr/weak_rc 实现、对象/控制块两阶段销毁、MSVC 对照 |
| 11 | 11-layout-and-representation.md | L11_layout | 布局/对齐/表示、数组与指针边界、standard-layout/trivially-copyable、EBO；观察 |
| 12 | 12-storage-and-object-creation.md | L12_storage | placement new/construct_at/destroy_at/implicit lifetime/start_lifetime_as/union；存储槽及重复占用边界 |
| 13 | 13-aliasing-and-provenance.md | L13_aliasing | 类型访问、bit_cast/memcpy、别名/launder、复用与指针来源；合法与错误对照 |
| 14 | 14-undefined-behavior-and-optimization.md | L14_ub | UB/IFNDR/实现定义/未指定、as-if、优化与诊断证据边界 |
| 15 | 15-object-buffer.md | P1_object_buffer | 受限资源容器，异常回滚/借用生命周期与下游回访 |

每题 README 逐 Part 写明前提、学生编辑位置、实际操作、Reference、检查命令、完整解析。实现型目录包含 src/student、reference、checks 与公开 validation 正反完成体；观察型没有伪造 Student 作业。作者每批同时交正文、代码、答案及证据。必要时可增补连续专题页，不能以固定篇幅删去知识。

C03 主讲完整类型/多态/错误设计，C04 主讲完整模板与查找/约束，C06 主讲容器/Ranges，C07 主讲 allocator/pmr/系统资源。C02 自己讲足所用的基本模板、异常和 allocator 用法，禁止循环先修。源码阅读穿插08/09/12/15，固定版本、问题、入口、状态与退出路径。

## 样章门

先交07“两个资源的RAII与部分构造失败”及其实际需要的00—06先修片段。正确手工 acquire/release 基线 → 新失败路径 → 有界事件/计数复现遗留资源和构造顺序 → 未完成对象及已完成成员析构规则 → RAII修复 → 重跑第k步失败 → std::unique_ptr 对照。

默认安全有限；真实悬空/双释放等放显式 unsafe 子进程。模型必须说明只证明模型，不能冒充实际 UB 崩溃。正文、Student、Reference、checker、解析、实验整体通过非作者教学/技术/实验审查及返修复验，才批量展开。

## 手写实现边界

最小 move-only 资源 owner；单线程 rc_ptr/weak_rc（strong=0销毁对象；strong和weak都为0才回收控制块；lock不能复活对象）。模型不实现原子、aliasing、enable_shared_from_this；标准能力以真实 std 用例与固定源码讲清。源码按本机 STL 版号、头文件 SHA 和独立上游提交分别绑定，不默认相同。

## object_buffer<T> 冻结契约

单线程、move-only，默认构造/析构、noexcept 移动构造/赋值、size/capacity、reserve(size_t)、push_back(T value)、pop_back、clear、view() -> span<const T>。不增加可写view、iterator或容器复制；深复制在05练习。

std::allocator<T>提供对齐存储，活跃对象仅在[0,size)。T完整、非cv对象、nothrow析构，且copy-constructible或nothrow-move-constructible；另外要求复制不改源、构造/移动维护类型不变量。trait只验证语法，不证明这些语义。throwing-move-only编译拒绝。

reserve无增长时无操作；超过allocator最大元素数抛length_error；增长防溢出。迁移用nothrow move否则copy，全部成功再提交。reserve及push_back函数体内失败保留原值/size/capacity/原借用，清理新对象。按值参数在函数前构造，保证不回滚实参求值及T对外部状态的副作用；自身const view元素复制追加须通过。

pop空抛out_of_range；clear析构对象但留容量。view不延长生命期；重分配销毁旧存储、pop/clear销毁相应元素才影响对应借用。move将源变空并转交allocation，源借用随新owner存活；move赋值先销毁目标原有数据，目标原借用失效。自move无损；未扩容append保留旧借用但不更新旧span长度。

## 公共构建接口

最低C++23/CMake3.28，整课及完整checkout内单题独立配置。Core自己的cmake/StudySetup.cmake，只读复用C01_Build_Compile_Link/exercises/include/check.hpp及需要时tools/process_runner.py；不修改C01 helper，不复制其课程专用大driver。代码纳入已有CTest模式，不加框架。

- core_configure_target(target)：C++23、无扩展、MSVC /utf-8 /EHsc /W4 /permissive- /Zc:__cplusplus；公共check头目录；可选安全ASan。
- core_add_test(name COMMAND ... LABELS ... TIMEOUT n)：默认命令为同名target，默认30秒，构建/诊断子任务180秒；不默认把77当成功。
- CORE_STUDY_BUILD_REFERENCE=ON；CORE_STUDY_TEST_STUDENTS=OFF；CORE_STUDY_ENABLE_UNSAFE_DEMOS=OFF；CORE_STUDY_ENABLE_FRONTIER=OFF；CORE_STUDY_ENABLE_ASAN=OFF。
- preset：verify-core(Release)、verify-debug(Debug)、student、asan、frontier，各自build目录。student关闭Reference并打开学生测试；任何Reference target、include、链接依赖不得进入Student。
- target采用单元_student/_reference/_observation；标签student/reference/observation/negative/capability。学生占位安全可编译，运行未实现必须明确失败；不得通过完成标记、SKIP或调用答案伪造完成。
- Release下check生效；源码错误阶段与诊断须匹配。超时、缺DLL、启动/清理失败独立FAIL，不能冒充预期负例。

每个作者维护叶级CMake，公共文件归leader；没有收到样章放行之前仅制作样章、必要先修和能力探测。

## 标准与前沿

主线固定[C++23 N4950](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4950.pdf)，C++20/23对比显式mode。C++26固定N5050，C++29固定N5054，[N5051](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html)和[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)确认其归属。规范状态与实现状态分两轴。

覆盖implicit move、range-for、start_lifetime_as及C++26初始化erroneous behavior/临时引用返回约束；N5055的基类designated初始化、default assignment限制、provenance/invalid pointer/lifetime-end DR在对应专题展开。DR不按年份机械改称全新特性。[trivial relocation历史](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3960r0.html)明确已移出C++26。源码、宏、真实实例化、链接、运行各自记录；未支持仍保留完整教学与实验，标环境受限。

## 验收与证据

1. 初始化/绑定编译正反、合法生命期序列；不硬断未指定顺序、sizeof、可选NRVO次数。return T{}删copy/move仍合法；named local同删应拒绝，另用有可用copy/move的类型观察NRVO。
2. 正常和第k次构造/复制失败，reset重复、move/selfmove；成功构造与析构匹配，资源归零。shared/weak两阶段、cycle/alias/lock；零/一/多元素、over-aligned/nontrivial对象与合法复用。
3. buffer空/边界/growth/自身copy追加、throwcopy回滚、nothrow move-only成功、不支持类型拒绝、非空目标移动赋值、源销毁、自move及借用变化。
4. 每实现型契约公开good通过、代表bad拒绝（恒值/no-op/假完成/旧值残留/漏析构/早提交/异常容量逃逸按题选）。隔离build复制完成体，保留Student学习状态。运行必须匹配预期checker诊断及非零；调用Reference另由构建依赖、预处理include和源码接线共同检查，公开bad接线例必须被拒。
5. Windows MSVC fresh Debug/Release、所有题叶级配置、Student隔离和正反；Clang ASan安全路径及独立heap-use-after-free故障匹配类别/位置/退出码。ASan无报告不证明没有UB。
6. 链接/锚点/coverage、四类下游回访：Coroutine的闭包/帧借用，Ranges借用不保活底层owner，Execution的operation state/receiver归属，并发回收中生命期与ABA区别。旧课只改README时验证导航及旧源码diff不变。

记录命令、输入、环境、flags、exit/timeout、stdout/stderr与源码/二进制指纹。默认仅以构造/复制/移动/分配计数解释当前类型路径，不宣称时间加速；如评价性能收益，先有可归因对照，再1预热+5独立进程，保留全部样本/无收益，不造benchmark框架。

## 所有权、审查与停止

leader管规格、coverage、Core公共构建/fixtures、导航、最终报告和复用helper指纹。三个作者分别00—06/L01—L06，07—10/L07—L10，11—15/L11—L14/P1；每批同时交正文、代码、解析、证据。两名非作者分教学和技术/实验审查，作者不得批准自己内容；返修由原reviewer复验，最终fresh集成另过审。

完整教学/Part/项目、约定支持路径验证、checker可信和下游回访完成，已知阻断清零并获非作者批准后结束。冻结源码与证据，回填本课质量报告和全局7.2仅C02本批。主线通过、前沿未验证及其他平台未测分别列，不以材料存在或SKIP冒充完成，不声明18课全局完成。
