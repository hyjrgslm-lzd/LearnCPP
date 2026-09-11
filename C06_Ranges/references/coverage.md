# C06覆盖、迁移与反向依赖

本表登记知识去向和检查入口，不以标题或测试数量宣告完成。规范归属与库能力在[标准索引](standards.md)，复验命令见练习构建指南。

## 新主线与下游能力

| 实际问题/下游 | 主讲位置 | 实现与检查 | 深度边界 |
|---|---|---|---|
| 给日志存储和查询选择容器 | [复杂度](../chapters/01-complexity-contracts.md) | L01计数；B01独立进程实验 | 区分最坏/平均/摊还；计数不是cache或总性能证明 |
| 在追加、随机访问、插删间取舍 | [序列容器](../chapters/02-sequence-containers.md) | L02 array/vector/deque/list/forward_list/stack/queue | 所有权、能力、失效与操作分布；不将实现布局当标准 |
| 返回span/view后还允许哪些修改 | [失效](../chapters/03-iterators-invalidation.md) | L02/L05、C2_2、H3 | 生命周期、引用、iterator与range对象分别判断 |
| 成长缓冲区的异常回滚 | [动态数组](../chapters/04-dynamic-array.md) | L05独立四路径；抛异常元素/移动/容量检查 | 有限dynamic_array；不承诺vector全API/allocator适配 |
| 有序用户报告与等价键 | [关联容器](../chapters/05-associative-containers.md) | L03、CAPSTONE1 | comp等价、multi、节点/透明查找；C03主讲错误建模 |
| 哈希索引在碰撞/rehash后不丢数据 | [关联容器](../chapters/05-associative-containers.md)、[机制实现](../chapters/07-heap-hash-avl.md) | L03/L07；B01查找与构建分离 | hash/equal前提、桶重建和异常；教学链式模型 |
| 正确排序、查找、分组和删除 | [算法](../chapters/06-algorithm-contracts.md) | L04、C2_1、CAPSTONE1 | 严格弱序、partition、稳定性、二分、merge/集合、erase/remove、fold/projection |
| 在线保留top-N与有序结构 | [堆/AVL](../chapters/07-heap-hash-avl.md) | L06/L08，独立结构检查和多个坏例 | 堆上浮/下沉、AVL子树重连/高度；不把sorted vector当结构通过 |
| 惰性解析被重复执行 | [演进样章](../chapters/00-log-pipeline-evolution.md) | CAPSTONE1相同链的16/9计数、独立good/bad；B01 | 先基线/复现/定位，再单遍或实体化；不预设收益 |
| 使用不同序列形状的组合 | 原01—07使用层正文 | warmup、A/B/C/D16观察入口 | 观察通过不自动表示扩展练习和解释完成 |
| 自定义range的合法表达式与语义 | 原08—10 | E1/F1/G1/G2/G3四路径；E2/E3观察 | 具体CPO、sentinel、closure、proxy/borrowed；通用模板回链C04 |
| 复制/移动缓存和单遍来源 | 原11 | H1/H2/H3四路径 | 缓存不变量、iter_move/iter_swap、句柄唯一所有权与引用 |
| 能阅读和实现一个受限序列库 | 原12与[源码路线](source-reading.md) | CAPSTONE3/CAPSTONE4 | 实际入口、状态/退出路径和六层实现，不以链接代替导读 |
| 有序紧凑、容量上限与稳定对象位置 | [新容器](../chapters/08-new-containers.md) | F01 flat核心与inplace/hive主体 | 真实规范与实现能力分开，缺能力保留完整主体 |
| 拼接/缓存/input降级/提示容量 | [前沿Ranges](../chapters/09-frontier-ranges.md) | F01 concat/cache_latest/as_input/const filter/reserve_hint | 新名字与DR单独记录；未实例化主体不算通过 |
| 用0/1范围、检查下标、无插入查找 | [前沿Ranges](../chapters/09-frontier-ranges.md) | F01 optional、C++29 at/lookup | C03状态先修；已入稿、提案、实测是不同状态 |

## 原12篇正文去向

| 原入口 | 去向与保留内容 | 对应练习 |
|---|---|---|
| 01 心智模型 | 原路径保留；range/view/iterator/sentinel、惰性、borrowed与所有权 | 01_mental_model_warmup |
| 02 模块A | 原路径保留；iota、istream、repeat、cartesian，空/有限/无限和消费 | A1/A2/A3 |
| 03 模块B | 原路径保留；all/ref/owning、filter/transform、take/drop与closure | B1/B2/B3 |
| 04 模块C1 | 原路径保留；join、split/lazy_split、common/reverse/elements，历史DR校正 | C1_1/C1_2/C1_3 |
| 05 模块C2 | 原路径保留；算法投影、返回值、borrowed/dangling、to | C2_1/C2_2/C2_3 |
| 06 模块D | 原路径保留；zip/相邻/分块/窗口、分组/拼接/const/rvalue及generator桥 | D1/D2/D3 |
| 07 项目1与源码路线 | 原路径保留；四字段日志项目，链接完整演进样章与固定源码入口 | CAPSTONE1 |
| 08 模块E | 原路径保留；访问CPO与算法对象、ADL隔离、历史tag_invoke对照 | E1/E2/E3 |
| 09 模块F | 原路径保留；iterator精化、关联类型、proxy与语义公理 | F1 |
| 10 模块G | 原路径保留；自写take/transform/enumerate、边界、closure与borrowed条件 | G1/G2/G3 |
| 11 模块H | 原路径保留；non-propagating cache、common/proxy、generator与const iterator | H1/H2/H3 |
| 12 实现项目 | 原路径保留；生产源码对照、六层mini-ranges | CAPSTONE3/CAPSTONE4 |

原索引的“31个子目录/4项目”不对应实际清单：本批开工是29个真实入口，其中CAPSTONE1/3/4为三个可执行项目。源码阅读路线继续保留，不额外冒充一项缺失的CAPSTONE2。本批不按数量删知识，也不将较难主题降为外链。

## 练习接线与证据含义

[39单元索引](../exercises/README.md)给出准确入口与分类。L05—L08、CAPSTONE1、E1、F1、G1—G3、H1—H3、CAPSTONE4承担独立实现；其余使用型入口提供完整观察与解析。L08另有假结构和陈旧高度控制，结构检查独立于被测实现的自报布尔值。

核心通过要求Reference和独立good正确、bad因目标缺陷被拒绝；Student初态独立失败并通过真实include接线审计。观察型检查分别列出，不将“代码可运行”偷换成“学习者已经完成所有Part”。全量命令、原始失败与复验见质量报告及各题作者记录。

## 先修与反向检查

- 数据结构实现使用C02存储与对象构造，但在动态数组正文重新连接到size/capacity、事务和失败路径，不要求尚未完整交付的C07作为硬先修。
- CPO、closure、proxy需要C04推导、ADL、约束、引用折叠和common_reference；本课必须继续解释range自己的协议，不能让学习者只照抄模板错误。
- 日志案例借用C03 optional/错误建模和C05字符串/解析知识；不在C06扩建完整CSV或网络系统。
- H3承接C09协程协议，但其句柄所有权、首次begin、yield引用和早销毁必须在本课有可运行边界实验。
- B01以本课操作为测量对象，C13主讲通用性能工程。没有计数或定位支持的成本方向保持假设，不从单组总耗时推断根因。

非作者教学验收应遮住Reference，沿以上先修完成代表性下游任务：实现短input范围的take、修复prvalue const iterator、维护AVL真实结构、解释日志复算与实体化代价。发现必须猜未讲规则时回补正文和解析，不能只新增一条通过记录。
