# C06 实施规格

用户于 2026-09-10 确认完整 C06 范围，并要求保留默认 C++26 构建；本文件将已批准的会话方案固定为实施与审查依据。

## 目标、边界与输入

面向已有基本 C++ 经验的工程师，建立从复杂度、容器和算法契约到 ranges 使用、实现与生产源码的连续课程。正文必须解释背景、前提、因果、反例、取舍与成本，练习检验这些知识，不能代替正文。完整保留原 01—12 文档及 29 个真实练习/项目入口，记录旧知识的去向。目录仍为 C06_Ranges。

允许修改 C06_Ranges/**、根 README 的 C06 导航和 LEARNCPP_GLOBAL_PLAN 的 C06 状态；CONTENT_REFACTORING_GUIDE 为只读验收依据。保留所有已有用户修改，不提交、不推送、不安装系统组件、不操作凭据和生产环境。

现状基线：MSVC 19.51.36256.0、MSVC STL 145 / 202604、CMake 4.2.3；默认 C++26 Release 生成 29 个程序，CTest 登记 0 个测试。G1、H3、CAPSTONE4 仅输出骨架提示。原始构建日志见 validation/baseline。原输入及其他用户文件指纹保存在本机忽略目录 .omx/context/c06-implementation，私人工作区清单不随教材发布。

## 路线与实现深度

1. 新增 chapters/ 基础主线：复杂度与摊还分析；容器契约、元素/allocator要求与异常；array/vector/deque/list/forward_list；stack/queue/priority_queue；map/set/multi；unordered/hash/equal/rehash；flat_map/flat_set/inplace_vector/hive；失效与借用；遍历、查找、变换、partition、排序与稳定性、二分、merge/集合、heap、erase/remove、fold与projection。
2. 保留原 Ranges 01—12 主线并纠正全体相关例子：view/all/ref/owning、工厂、结构适配、sentinel、borrowed/dangling、const、proxy、iterator_concept/category、closure、CPO、non-propagating cache、自定义视图和mini-ranges。
3. 机制实现限于教学需要：小型动态数组的增长和异常回滚、二叉堆、链式哈希表的碰撞/rehash、AVL插入/旋转与平衡不变量；每个实现明确支持的操作和类型，不承诺STL兼容。复杂容器实现细节通过固定MSVC源码路径和阅读任务展开。
4. C++26新容器、concat/cache_latest/to_input/reserve_hint及相关范围能力提供完整解释、真实实验主体和解析；宏、头、实例化、链接和运行分别验证。环境不足保持SKIP，不能只有探测宏而没有主体。C++29按固定草案登记已入稿与提案，不预言最终标准。
5. C02主讲对象/资源，C04主讲通用模板与定制，C05主讲解析/数据表达，C09主讲协程语言协议；本课保留range具体实现、yield引用和迭代的必要推导。C13拥有通用性能工程，本课仍完成自身操作计数和归因实验。

## 主案例与样章门槛

沿用CAPSTONE1四字段 timestamp,level,user_id,message 日志子集及非法行过滤行为，不增加action字段或完整CSV解析器。主案例：显式循环正确基线→vector记录存储→有序/无序索引→heap top N→Ranges查询与收束。必须说明相同契约的替代实现及改变场景的分支。

首个样章为“解析结果经过transform/filter的重复计算与临时对象借用”。完整交付正确循环基线、安全按值惰性版本、操作计数、失效/编译反例、独立Student/Reference/checker和解析。先留下复现与归因，再比较单遍消费/实体化；不预设加速比。样章接受非作者审查并修复复验后才扩展作者批次。

## 公共构建和练习契约

保留 exercises/ 和单题CMake入口、默认C++26及vs2026预设。公共构建由主负责人维护。复用 C01_Build_Compile_Link/exercises/include/check.hpp 的Release有效check；禁止复制替代检查框架。借鉴C04/C05最小CMake模式，不直接耦合其课程特定函数。

RangesSetup.cmake提供：

- ranges_configure_target(target)：C++26、MSVC选项、共享check include及可选ASan。
- ranges_add_test(name COMMAND ... LABELS ... TIMEOUT n)：默认30秒、真实CTest登记。
- ranges_add_observation(name source)：完整可运行观察程序，仅声明实际验证范围。
- ranges_add_exercise(NAME name HEADER file CHECK checks.cpp BAD_DIAGNOSTIC text)：构建src/student、src/reference、validation/good、validation/bad四种独立实现，checker按include路径选择同名接口。
- RANGES_BUILD_REFERENCE默认ON，RANGES_TEST_STUDENTS默认OFF，RANGES_ENABLE_FRONTIER默认OFF；保留warnings/ASan/UBSan现有选项。

关键实现题采用独立四路径与真实接口检查；Student初态安全、有限、明确失败，不能修改完成标记通过。bad必须因预期check诊断退出1，任意崩溃/超时不能算拒绝成功。观察型题有完整程序、预测/解释和答案；观察通过不代表实现练习完成。每题独立配置与构建；旧main入口保留明确用途，不把skeleton compile记为行为通过。

## 验证与交付

- 核心Debug/Release、独立ASan安全集；所有自动运行有进程外超时。
- 容器：空/单元素、重复、容量边界、碰撞、rehash、元素抛异常和失效；算法：前提、稳定性与结果边界；Ranges：input-only、非common/sized、proxy/prvalue、move-only、borrowed/cache、generator移动/提前销毁/异常。
- Reference与独立good通过；bad有效拒绝；未完成Student原始运行失败；编译include路径验证Student隔离。
- 性能限定日志查询/收束、索引构建/查找两组，先计数定位再计时。固定输入/种子、不同规模与选择/命中率；一次预热、五次独立进程，保存全部样本、中位数和范围。结果正确性独立于计时，负结果不删除。
- 保存coverage、标准/实现索引、源码阅读路径、validation原始命令/退出码/输出、benchmarks和quality-report。来源/源码/数据绑定指纹，PASS/FAIL/SKIP/未验证明确区分。
- 所有正文自测与关键Part有解析；非作者遮住答案验证可做性；样章、每批和集成均审查教学/技术/实验，修复后复验。

## 所有权与停止条件

先基线/样章与规格审查，后并行主题作者：基础容器算法、Ranges使用、Ranges实现。主负责人持有公共构建、导航、标准索引、共享输入、验证工具及最终报告。作者不修改其他作者文件，不批准自己写的内容。专用architect在此前只读阶段因模型不可用失败，不能伪称正式ralplan/architect门槛通过；实施采用可用非作者完成教学/技术/实验审查并如实记录角色。

停止条件：完整C06覆盖闭合、全部旧知识有去向、所有已知阻断修复并独立复验、约定矩阵完成、环境限制与未测内容显式登记。存在未完成批次不得以局部通过宣称整课完成。

## 实施中的一手规范校正

2026-09-10查得N5047已采纳P3828R1、P3725R3、P3981R2：本规格中to_input按当前名as_input实施并保留历史对照；filter新增受约束const/input支线；inplace_vector的try接口当前返回optional<T&>。这是原前沿能力的版本校正，不扩大范围。依据和精确边界见standards.md，后续实验与非作者技术审查同步覆盖。
