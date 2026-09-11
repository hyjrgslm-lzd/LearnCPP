# C03 覆盖与学习入口

本表按能力追踪，不以题数代替完整性。教学解析在各正文和练习README；每个实现单元的checks消费选中的Student/Reference/good/bad。观察单元不宣称学生实现完成。本表不记录本机验证日志、审查记录、交付清单或测试计数。

| 下游场景/核心问题 | 主讲与先修 | 实验/实现/解析 |
|---|---|---|
| 输入进入值对象后维护有效状态 | [01](../chapters/01-class-invariants.md)，C02初始化/封装 | [L01](../exercises/L01_invariants/README.md)，构造拒绝、独立实现和bad |
| 复制、身份、比较与hash保持语义一致 | [02](../chapters/02-value-semantics.md)，值/资源基础 | [L02](../exercises/L02_value_semantics/README.md)，货币/值比较及错误组合拒绝 |
| 查找缺项、缓存与可空借用 | [03](../chapters/03-optional-and-empty-state.md)，C02生命周期 | [L03](../exercises/L03_optional/README.md)，monadic与emplace失败；[F01](../exercises/F01_frontier/README.md)引用/range |
| 协程/迭代器/命令的封闭状态集合 | [04](../chapters/04-variant-and-state-space.md)，类型与值类别 | [L04](../exercises/L04_variant/README.md)，visit/get/重复类型/valueless；F01成员visit |
| 同步命令、异步值/错误载荷、工厂 | [05](../chapters/05-expected-and-error-channels.md)，03/04 | [L05](../exercises/L05_expected/README.md)，expected/void/monadic/借用、引用T真实编译拒绝 |
| 文档批处理失败后状态责任 | [06](../chapters/06-exception-safety-and-transactions.md)，C02复制/RAII及01/02 | [L06](../exercises/L06_transactions/README.md)，basic/strong、两种正确算法、逐失败点、两个隔离bad |
| 容器/回调接口的借用和快照 | [07](../chapters/07-interfaces-and-polymorphism.md)，C02寿命 | [L07](../exercises/L07_interfaces/README.md)，当前实现rvalue能力检查、独立bad、编译诊断 |
| 组件组合、继承与替换性 | [08](../chapters/08-composition-and-inheritance.md)，01/07 | [L08](../exercises/L08_composition/README.md)，NVI和构造期派发 |
| 开放对象复制与销毁 | [09](../chapters/09-dynamic-polymorphism.md)，08及C02拥有 | [L09](../exercises/L09_dynamic_polymorphism/README.md)，动态类型clone/独立所有权/两个bad |
| 编译期选择与受约束接口 | [10](../chapters/10-static-polymorphism.md)，必要模板本章先讲 | [L10](../exercises/L10_static_polymorphism/README.md)，requires/CRTP/this-> |
| any_view/可调用对象的擦除责任 | [11](../chapters/11-type-erasure.md)，09/10 | [L11](../exercises/L11_type_erasure/README.md)，拥有型heap-only AnyShape、clone/销毁/异常/对齐/空态 |
| 同步借用与持有回调 | [12](../chapters/12-callable-objects-and-type-erasure.md)，07/11 | [L12](../exercises/L12_callables/README.md)，C23四类实际调用；F01 C26包装 |
| 动态存储的独立复制值 | [13](../chapters/13-indirect-values-and-polymorphic-ownership.md)，C02/09/11 | [L13](../exercises/L13_indirect_values/README.md)，明确命名教学模型；F01真实std设施 |
| 输入边界与程序契约 | [14](../chapters/14-contracts-hardening-and-interface-boundaries.md)，01/05/08 | [L14](../exercises/L14_contracts/README.md)，安全模型；F01语言/库/虚函数契约 |
| 旧客户端与新接口保持兼容 | [15](../chapters/15-interface-evolution-and-compatibility.md)，07/09及C01桥接 | [L15](../exercises/L15_evolution/README.md)，真实适配调用与默认实参负例，ABI风险边界 |
| 上述公共能力组合成库 | [16](../chapters/16-document-model-and-transactions.md)，01—07和必要visitor机制 | [P1](../exercises/P1_document/README.md)，事务/快照/复制/实际分配注入 |
| 源码和真实下游反向核对 | [17](../chapters/17-source-reading-and-downstream-bridges.md)，对应主讲章 | 固定源码入口、C06/C09/C10既有调用；结课问答解析 |

## 原知识去向与边界

本任务新增C03，未搬走或删除旧课知识。C02继续主讲生命周期/资源，C06继续主讲range语义，C09/C10继续主讲各自异步协议；新主讲与旧课README回链对应。量纲在01及标准索引承接类型建模，数值算法归C13；模式匹配未采纳提案在14/索引跟踪，不冒充标准实现。不能从未交付目录推断先修已齐备。

所有自测及必做Part均需在其正文/README中找到解析；未完成的Student、观察程序、Reference、good/bad控制和前沿能力各有不同结果含义。运行记录和本机指纹属于过程产物，不进入稳定课程文档。
