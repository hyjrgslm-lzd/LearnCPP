# C03：类型建模与接口设计

一个对象“内存合法”并不代表它的业务状态有效；一个函数“返回错误”也不代表修改已经回滚。C03从这些差异出发，把类不变量、值语义、状态、错误、多态和接口演进连成一条路线。

读者应能编写基本现代C++，具备[C01](../C01_Build_Compile_Link/README.md)的基本构建能力，以及[C02](../C02_Objects_Lifetime_Ownership/README.md)的对象、生命周期与RAII基础。所用局部模板机制在使用前解释，完整泛型体系由C04后续主讲。

贯穿案例是内存中的图形文档：有效尺寸与唯一ID、可复制值、缺值/错误返回、事务修改、开放或封闭扩展、接口兼容。复杂机制用小实验拆开验证；最终项目选择variant，不把每一种多态包装塞入同一个类。

## 阅读路线

| 章 | 正文 | 要解决的问题 |
|---|---|---|
| 00 | [路线与边界](chapters/00-route-and-boundaries.md) | 类型和接口究竟承诺什么 |
| 01 | [类不变量](chapters/01-class-invariants.md) | 怎样让非法状态不能悄悄进入有效对象 |
| 02 | [值语义](chapters/02-value-semantics.md) | 复制、身份、相等、排序和hash怎样一致 |
| 03 | [optional](chapters/03-optional-and-empty-state.md) | 缺值、借用和0/1范围各是什么 |
| 04 | [variant](chapters/04-variant-and-state-space.md) | 怎样声明并访问封闭状态集合 |
| 05 | [expected与错误](chapters/05-expected-and-error-channels.md) | 怎样分类业务拒绝与异常传播 |
| 06 | [异常安全与事务](chapters/06-exception-safety-and-transactions.md) | 失败时对象还满足什么保证 |
| 07 | [参数与返回接口](chapters/07-interfaces-and-polymorphism.md) | 借用、快照、const/ref/noexcept如何选择 |
| 08 | [组合与继承](chapters/08-composition-and-inheritance.md) | 结构复用与替换性是否一致 |
| 09 | [动态多态](chapters/09-dynamic-polymorphism.md) | 派发、析构和动态类型复制由谁负责 |
| 10 | [静态多态](chapters/10-static-polymorphism.md) | 模板、约束与CRTP分别做什么 |
| 11 | [类型擦除](chapters/11-type-erasure.md) | 怎样隐藏具体类型而保留拥有与操作责任 |
| 12 | [可调用包装](chapters/12-callable-objects-and-type-erasure.md) | 拥有/借用、const调用与空调用有何不同 |
| 13 | [间接值](chapters/13-indirect-values-and-polymorphic-ownership.md) | 如何获得动态存储对象的独立值语义 |
| 14 | [契约与hardening](chapters/14-contracts-hardening-and-interface-boundaries.md) | 输入验证、断言和语言/库检查如何分工 |
| 15 | [接口演进](chapters/15-interface-evolution-and-compatibility.md) | 源、行为与ABI兼容为何分开验证 |
| 16 | [文档模型集成](chapters/16-document-model-and-transactions.md) | 怎样把上述保证组合成完整库接口 |
| 17 | [源码与下游回访](chapters/17-source-reading-and-downstream-bridges.md) | 怎样沿实现和真实应用反查知识深度 |

00—07建立值/状态/错误/接口基础；08—13比较不同扩展和拥有契约；14—15讨论边界与演进；16—17集成并回访。实际硬先修按章声明，编号不意味着后续案例必须使用所有较早机制。

## 实验与交付证据

[BUILD_GUIDE](exercises/BUILD_GUIDE.md)提供整课、单题、Student、ASan与frontier入口。Student、Reference、观察程序和good/bad控制含义分明：初始Student明确失败，不能靠返回success或完成标记冒充实现；观察程序通过不代替预测与解析。

核心使用C++23。本机支持不足的C++26/29内容仍有正文、真实源码与独立能力探测；标准地位、实现能力与实际运行结果分别记录在[标准索引](references/standards-and-implementations.md)。性能没有预设排名，复制/分配计数只用于对应受控实验。

本课已完成教材、Windows可用路径验证与非作者独立终审，实际结果和环境限制见[质量报告](references/quality-report.md)。[实施规格](references/implementation-spec.md)说明承诺范围，[覆盖表](references/coverage.md)连接各知识点、练习与审查证据；[交付清单](references/delivery-manifest.md)绑定文件与证据版本。能力SKIP不计为设施运行通过。

## 下游衔接

- [C06](../C06_Ranges/README.md)：optional缓存、variant状态与类型擦除；range协议由C06主讲。
- [C09](../C09_Coroutines/README.md)：值/异常状态、回调借用；帧与恢复协议由C09主讲。
- [C10](../C10_Execution/README.md)：完成通道和可调用对象责任；运行时/调度协议由C10主讲。
- C01深入ABI/工具链，C02深入对象/存储，C04深入泛型/反射，C13深入量纲/数值和性能；本课不给尚未交付课程制造虚假文件链接。
