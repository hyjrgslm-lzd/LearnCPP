# C06练习索引

共39个课程单元：原29个入口保留，增加8个基础单元、F01和B01。原索引的“31个”是计数错误，没有因此删除两道题。读取[课程路线](../README.md)和[构建指南](BUILD_GUIDE.md)，先读正文再运行或实现。

## 分类与入口

“观察”表示完整程序只验证运行中实际检查的行为，仍须回答题面的预测/扩展与解析；“实现”表示Student独立完成操作，Reference与good/bad用于校验。能力SKIP不是实现完成。具体CHECK源文件和编辑位置以每题README/CMake为准，历史骨架说明不能作为行为通过记录。

| 入口 | 分类 | 核心问题 |
|---|---|---|
| [L01_complexity](L01_complexity/README.md) | 观察 | 最坏、平均与摊还的操作计数 |
| [L02_sequence_containers](L02_sequence_containers/README.md) | 观察 | 序列容器、span、stack与queue |
| [L03_associative_containers](L03_associative_containers/README.md) | 观察 | 比较等价、节点、透明查找、hash/equal |
| [L04_algorithms](L04_algorithms/README.md) | 观察 | 排序、partition、二分、集合、heap、fold |
| [L05_dynamic_array](L05_dynamic_array/README.md) | 实现 | 对象/存储分离、增长与异常回滚 |
| [L06_binary_heap](L06_binary_heap/README.md) | 实现 | 上浮、下沉、容量边界 |
| [L07_chained_hash](L07_chained_hash/README.md) | 实现 | 碰撞、更新、删除与rehash |
| [L08_avl_tree](L08_avl_tree/README.md) | 实现 | 旋转、实际树结构与独立不变量检查 |
| [01_mental_model_warmup](01_mental_model_warmup/README.md) | 观察 | range/view/sentinel与借用 |
| [A1_iota_view](A1_iota_view/README.md) | 观察 | 有界与无界iota |
| [A2_istream_view](A2_istream_view/README.md) | 观察 | input-only、消费与物化 |
| [A3_repeat_cartesian](A3_repeat_cartesian/README.md) | 观察 | repeat、cartesian与空输入 |
| [B1_all_ref_owning](B1_all_ref_owning/README.md) | 观察 | all/ref/owning与对象依赖 |
| [B2_filter_transform_degrade](B2_filter_transform_degrade/README.md) | 观察 | 能力裁剪、const与求值 |
| [B3_take_drop_closure](B3_take_drop_closure/README.md) | 观察 | take/drop/while与closure |
| [C1_1_join](C1_1_join/README.md) | 观察 | 内层范围与join |
| [C1_2_split_evolution](C1_2_split_evolution/README.md) | 观察 | split/lazy_split与C++20 DR |
| [C1_3_common_reverse_elements](C1_3_common_reverse_elements/README.md) | 观察 | common/reverse/elements |
| [C2_1_projection](C2_1_projection/README.md) | 观察 | 投影与算法返回类型 |
| [C2_2_dangling_borrowed](C2_2_dangling_borrowed/README.md) | 观察 | 右值算法与borrowed/dangling |
| [C2_3_ranges_to](C2_3_ranges_to/README.md) | 观察 | 实体化、from_range与继续组合 |
| [D1_zip_adjacent_chunk](D1_zip_adjacent_chunk/README.md) | 观察 | zip、相邻与窗口/分块 |
| [D2_chunk_by_join_with_asconst](D2_chunk_by_join_with_asconst/README.md) | 观察 | 分组、分隔拼接、const与rvalue元素 |
| [D3_std_generator](D3_std_generator/README.md) | 观察 | 真实std::generator与elements_of |
| [CAPSTONE1_log_pipeline](CAPSTONE1_log_pipeline/README.md) | 实现 | 完整日志查询及样章演进 |
| [E1_my_begin_cpo](E1_my_begin_cpo/README.md) | 实现 | array/member/ADL与借用约束 |
| [E2_niebloid](E2_niebloid/README.md) | 观察 | 算法函数对象的版本语义 |
| [E3_cpo_tagdispatch_compare](E3_cpo_tagdispatch_compare/README.md) | 观察 | range定制与历史tag_invoke对照 |
| [F1_iterator_hierarchy](F1_iterator_hierarchy/README.md) | 实现 | 迭代器精化与真实操作 |
| [G1_my_take_view](G1_my_take_view/README.md) | 实现 | 有限消费、unsized与sentinel |
| [G2_my_transform_closure](G2_my_transform_closure/README.md) | 实现 | callable、值类别和closure组合 |
| [G3_my_enumerate_borrowed](G3_my_enumerate_borrowed/README.md) | 实现 | 索引、proxy与借用透传 |
| [H1_non_propagating_cache](H1_non_propagating_cache/README.md) | 实现 | 缓存复制/移动与view状态 |
| [H2_common_iter_proxy](H2_common_iter_proxy/README.md) | 实现 | common_iterator与iter_move/iter_swap |
| [H3_generator_const_iter](H3_generator_const_iter/README.md) | 实现 | 帧所有权与const iterator引用 |
| [CAPSTONE3_impl_source_reading](CAPSTONE3_impl_source_reading/README.md) | 观察 | 固定实现源码阅读及解释 |
| [CAPSTONE4_mini_ranges](CAPSTONE4_mini_ranges/README.md) | 实现 | 六层受限mini-ranges |
| [F01_frontier](F01_frontier/README.md) | 观察＋前沿能力 | 新容器、C++26/C++29真实主体与能力 |
| [B01_cost](B01_cost/README.md) | 正确性快测＋性能实验 | 日志和索引的诊断与独立进程测量 |

## 实现型练习的四条路径

- `src/student/`：学习者独立编辑的位置；初态安全、有限且真实失败。
- `src/reference/`：完整参考实现，与Student分开。
- `validation/good/`：独立正确控制体，不能调用Reference。
- `validation/bad/`：实际行为错误的控制体；必须因预期诊断被拒绝，不能把超时或崩溃当成功。

同一checker通过目标的include路径选择实现，并调用其实际接口。类型约束检查与运行时检查互补；声明iterator tag本身不证明concept和语义成立。算法结构还需要实际结构检查，不能由`is_balanced()`之类的完成声明代替。

默认构建不运行未完成Student。按题面显式构建`<unit>_student`；旧目标名保留为该Student的构建入口。未完成返回失败是预期学习状态，改完后应让原checker通过；不要修改checker的预期值或借用Reference实现。

## 三个保留的结课项目

CAPSTONE1是完整日志管道，CAPSTONE3是实现级源码阅读，CAPSTONE4是六层mini-ranges。旧编号保留，但当前不存在一个被遗漏的CAPSTONE2作业；旧正文中的“源码阅读路线”不再额外计为一个可执行项目。

## 验证边界

默认C++26是构建选择，不表示全部库功能已实现。`split`的历史设计差异按DR和实际实现解释；view可拥有元素，不能背成全部copy/move/destroy严格O(1)；borrowed不延长owner存活。细节见[标准索引](../references/standards.md)。

每题正文与README提供解析；[覆盖表](../references/coverage.md)将下游任务反向连回知识。性能结论只来自可复现的[B01题面](B01_cost/README.md)方法，不能用临时运行日志替代。
