# C02 覆盖与证据入口

本表按主讲责任连接正文、实际练习与独立审查；状态必须以绑定源码的报告为准。目录、作者完成和观察运行都不能代替教学验收。全部主讲单元已分批独立审查，最终整合状态以质量报告签署为准。

| 单元 | 主讲能力与下游用途 | 正文 / 练习入口 | 当前独立状态 |
|---|---|---|---|
| 00 | 区分类型、对象、存储、作用域、生命期和资源责任 | [正文](../chapters/00-model-and-route.md) | [独立审查通过](validation/reviews/reviewer-c02-l07-r3-final.md) |
| 01 | 判断初始化和求值规则；为接口、构造与返回建立前提 | [正文](../chapters/01-initialization.md) / [L01](../exercises/L01_initialization/README.md) | [独立审查通过](validation/reviews/reviewer-foundation-01-06-r1.md) |
| 02 | 从值类别、cv、绑定与decltype解释调用；支撑泛型/await协议 | [正文](../chapters/02-expressions-and-references.md) / [L02](../exercises/L02_value_categories/README.md) | [独立审查通过](validation/reviews/reviewer-foundation-01-06-r1.md) |
| 03 | 追踪owner与borrower生命期；应用于闭包、帧与view | [正文](../chapters/03-lifetime-and-borrowing.md) / [L03](../exercises/L03_lifetimes/README.md) | [独立审查通过](validation/reviews/reviewer-foundation-01-06-r1.md) |
| 04 | 解释部分构造/异常展开/委托异常，正确清理已完成成员 | [正文](../chapters/04-construction-and-unwinding.md) / [L04](../exercises/L04_construction/README.md) | [独立审查通过](validation/reviews/reviewer-c02-l07-r3-final.md) |
| 05 | 解释特殊成员生成和深复制；保证值与资源责任不混淆 | [正文](../chapters/05-special-members.md) / [L05](../exercises/L05_special_members/README.md) | [独立审查通过](validation/reviews/reviewer-foundation-01-06-r1.md) |
| 06 | 区分move转换/实际移动/必然消除/NRVO，推导异常保证 | [正文](../chapters/06-move-and-return.md) / [L06](../exercises/L06_move_return/README.md) | [独立审查通过](validation/reviews/reviewer-foundation-01-06-r1.md) |
| 07 | 资源及时进入owner，成功/失败/move均正确收束 | [正文](../chapters/07-raii-and-ownership.md) / [L07](../exercises/L07_raii/README.md) | [独立审查通过](validation/reviews/reviewer-c02-l07-r3-final.md) |
| 08 | 正确使用unique_ptr/deleter/数组/不完整类型及释放转交 | [正文](../chapters/08-unique-ownership.md) / [L08](../exercises/L08_unique/README.md) | [独立审查通过](validation/reviews/verifier-08-10-r2.md) |
| 09 | 分清存储指针与控制块、循环、weak lock及同步边界 | [正文](../chapters/09-shared-ownership.md) / [L09](../exercises/L09_shared/README.md) | [独立审查通过](validation/reviews/verifier-08-10-r2.md) |
| 10 | 实现并解释单线程强弱计数两阶段销毁，与生产实现对照 | [正文](../chapters/10-control-block.md) / [L10](../exercises/L10_control_block/README.md) | [独立审查通过](validation/reviews/verifier-08-10-r2.md) |
| 11 | 根据类型/标准而非某次地址推导对齐、表示和指针边界 | [正文](../chapters/11-layout-and-representation.md) / [L11](../exercises/L11_layout/README.md) | [独立审查通过](validation/reviews/reviewer-storage-11-15-r2-final.md) |
| 12 | 区分分配与对象创建；合法构造、销毁、隐式创建和union切换 | [正文](../chapters/12-storage-and-object-creation.md) / [L12](../exercises/L12_storage/README.md) | [独立审查通过](validation/reviews/reviewer-storage-11-15-r2-final.md) |
| 13 | 解释类型访问、字节复制、复用、launder及来源规则 | [正文](../chapters/13-aliasing-and-provenance.md) / [L13](../exercises/L13_aliasing/README.md) | [独立审查通过](validation/reviews/reviewer-storage-11-15-r2-final.md) |
| 14 | 判断规范状态与实验边界，避免从无诊断推导无UB | [正文](../chapters/14-undefined-behavior-and-optimization.md) / [L14](../exercises/L14_ub/README.md) | [独立审查通过](validation/reviews/reviewer-storage-11-15-r2-final.md) |
| 15 | object_buffer整体状态、异常回滚、移动与借用有效性 | [正文](../chapters/15-object-buffer.md) / [P1](../exercises/P1_object_buffer/README.md) | [独立审查通过](validation/reviews/reviewer-storage-11-15-r2-final.md) |

## 下游反向验收

| 下游实际问题 | C02应提供的解释 | 应用侧保留责任 |
|---|---|---|
| 协程lambda闭包先析构，帧后恢复 | 03/04/07解释引用不保活、闭包对象与资源owner | Coroutine解释协程帧、初始挂起、恢复和销毁协议 |
| borrowed_range存在但底层owner已死 | 02/03/11解释引用/指针与活跃对象关系 | Ranges解释borrowed_range/view/iterator各自语义和失效契约 |
| operation state没有稳定持有receiver | 04—10解释构造完成、所有权转交及异常路径 | Execution解释start/completion/scope以及完成后的收束 |
| 地址复用、ABA与回收 | 11—14解释存储复用及对象身份、合法访问条件 | Concurrency解释发布、同步、算法历史和安全回收协议 |

## 固定版本与能力状态

规范/实现索引独立记录每项版本与DR。已执行的初始能力调查见[probe摘要](validation/author-storage-probes/results-20260908.md)，源码、实际命令和raw JSON在同目录；宏检查不能冒充完整行为检查。最终质量报告还须把新增正文/练习与这些前置探测分别绑定，不能只复用本表的一句“已探测”。
