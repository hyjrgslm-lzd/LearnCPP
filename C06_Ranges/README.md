# C06：数据结构、容器、算法与 Ranges

这门课从“数据怎样存、操作需要什么前提”进入“序列怎样被惰性消费”，再深入迭代器、视图和关键数据结构的实现。面向已有基本 C++ 经验的工程师；正文提供机制推导，练习验证理解，源码阅读和实验检验设计取舍。

默认构建保持 **C++26 / MSVC `/std:c++latest`**。知识分别标明 C++20/23、C++26/C++29 草案及追溯缺陷修正；语言模式、标准库声明和实际运行能力分别验证。交付状态与限制见[质量报告](references/quality-report.md)，覆盖与迁移见[覆盖表](references/coverage.md)。

## 从哪里开始

C01 提供最小构建与调试，C02 提供对象/资源基础，C03 提供状态和错误建模。实现自定义 view 前按需补 C04 的推导、查找和约束；不要求先学完 Execution、协程或 C07 才能使用容器。

| 顺序 | 正文入口 | 完成后的能力 |
|---|---|---|
| 1 | [复杂度与契约](chapters/01-complexity-contracts.md) | 区分最坏、平均、摊还，先定义完整操作再谈成本 |
| 2 | [序列容器](chapters/02-sequence-containers.md)、[迭代器与失效](chapters/03-iterators-invalidation.md) | 选择存储方式，解释元素、引用、iterator和owner关系 |
| 3 | [动态数组](chapters/04-dynamic-array.md) | 推导存储/对象分离、增长事务、异常回滚和类型边界 |
| 4 | [关联容器](chapters/05-associative-containers.md)、[算法契约](chapters/06-algorithm-contracts.md) | 维护比较/hash不变量，正确使用排序、查找、集合、heap和fold |
| 5 | [堆、哈希与AVL](chapters/07-heap-hash-avl.md) | 实现有限数据结构并验证结构不变量 |
| 6 | [Ranges心智模型](01-心智模型.md)至[日志项目](07-结课项目1-数据管道与源码阅读.md) | 组合并消费视图，解释类型能力、借用和求值时机 |
| 7 | [日志管道演进样章](chapters/00-log-pipeline-evolution.md) | 从真实重复求值证据选择单遍消费或实体化，并解释代价 |
| 8 | [CPO](08-模块E-CPO与niebloid.md)、[迭代器精化](09-模块F-概念精化与迭代器分类.md)、[自写视图](10-模块G-自行实现视图.md)、[高级模式](11-模块H-高级实现模式.md)、[实现项目](12-结课项目2-实现级源码阅读.md) | 设计range协议、缓存和proxy，完成受限mini-ranges |
| 9 | [新容器](chapters/08-new-containers.md)、[前沿Ranges](chapters/09-frontier-ranges.md) | 对照新契约与本机实现，准确解释能力跳过 |

原有12篇文档保留路径；新增正文放在chapters。样章的00是评审样本编号，学习次序以上表的先修链为准。

## Ranges使用层入口

- [视图工厂与惰性](02-模块A-视图工厂与惰性.md)：iota、istream、repeat与cartesian。
- [基础适配器与管道](03-模块B-基础适配器与管道.md)：all/ref/owning、filter/transform、take/drop及closure。
- [结构适配器](04-模块C1-结构适配器.md)：join、split/lazy_split、common/reverse/elements。
- [算法、投影与范围边界](05-模块C2-算法·投影·范围边界.md)：算法前提、projection、dangling/borrowed与ranges::to。
- [高阶视图与协程桥](06-模块D-C++23高阶视图与协程桥.md)：zip、窗口/分块/分组与真实std::generator。

## 一个主案例与多个机制实验

沿用`timestamp,level,user_id,message`四字段日志子集：显式循环正确基线、记录存储、频次与有序报告、Ranges查询与收束。CAPSTONE1中的解析、借用、重复求值和实体化有连续推导及真实检查。堆、哈希、AVL、缓存、迭代器等用局部实验隔离机制，不强行把每种数据结构塞进同一业务系统。

[成本实验](references/benchmarks/README.md)比较日志查询/收束与索引构建/查找。它们有相同契约下的对照，也明确标记独立整数键机制分支；先取得正确性与操作计数，再记录时间，不预设某个容器或Ranges更快。

## 练习怎么做

进入[练习索引](exercises/README.md)与[构建指南](exercises/BUILD_GUIDE.md)。39个课程单元保留原29个入口并增加8个基础单元、F01和B01；数量不是完成度证明。

- **观察型**：完整安全程序，用实际check验证所声明行为；运行通过不代替预测、解释和扩展，解析在正文和题面。
- **实现型**：编辑题内`src/student/`，按题面列出的checker消费当前实现。Reference、独立good和真实bad分开；初态Student可以编译，但必须因未完成行为失败。
- **前沿实验**：选项关闭、能力SKIP和真正FAIL分开。缺失主体保留代码，不用自写替代包装伪称标准库实现。

```powershell
cd C06_Ranges/exercises
cmake --preset vs2026
cmake --build --preset vs2026
ctest --preset core-release
```

默认验证Reference、观察程序、独立good/bad；Student单独显式构建运行。Debug、ASan、Student隔离和前沿预设见构建指南。其他保留预设不自动意味着本批验证了对应平台。

## 实现边界与源码阅读

动态数组、二叉堆、链式哈希、AVL只实现题面声明的有限操作；mini-ranges保留CPO、concept、view interface、factory、adaptor、consumer六层主线。教学AVL不是MSVC map的红黑树，函数名相似也不等于标准库兼容性。

[源码阅读路线](references/source-reading.md)固定本机STL版本与指纹，从vector增长、树旋转、hash rehash和ranges缓存的真实入口追踪状态与退出路径。[标准索引](references/standards.md)分别记录规范、DR、提案和实际实现；不要把旧提案名字、别家库接口或滚动源码当成同一版本。

## 跨课责任

C04主讲通用模板/查找/约束，C06承接具体range协议；C09主讲协程语言机制，C06承接generator的range与yield引用；C13主讲通用性能工程，C06完成自己的可归因成本实验。解析和格式细节回链C05；本课不添加网络、并发、GPU或第三方解析依赖。

## C02 引用与借用先修

[表达式与引用](../C02_Objects_Lifetime_Ownership/chapters/02-expressions-and-references.md)和[生命周期与借用](../C02_Objects_Lifetime_Ownership/chapters/03-lifetime-and-borrowing.md)为 view、iterator 与惰性访问提供先修。borrowed_range 讨论迭代器是否依赖 range 对象本身，不会自动延长底层 owner 的生命期；具体视图、迭代器与失效契约继续由本课主讲。

## C03 状态与类型擦除衔接

[C03 optional](../C03_Type_Modeling_Interface_Design/chapters/03-optional-and-empty-state.md)、[variant](../C03_Type_Modeling_Interface_Design/chapters/04-variant-and-state-space.md)及[类型擦除](../C03_Type_Modeling_Interface_Design/chapters/11-type-erasure.md)补齐缓存、判别状态和any_view所需基础。C++26 optional的0/1范围含义由C03引入，具体range协议在本课承接；擦除保留哪些迭代能力、引用和失效保证需要另行规定，不能自动推出稳定ABI。

## C04 泛型与编译期桥接

[进入C04课程](../C04_Generic_CompileTime_Reflection/README.md)。实现G1/G2自定义view和H2 proxy iterator前，按需补读C04的推导/转发、查找/ADL、约束、tuple与CPO。具体range协议、借用和迭代器语义仍在本课展开。
