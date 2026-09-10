# C++29 并发前沿：线程属性与 HP 批量句柄

本专题只讲已经进入 N5054 工作草案的 C08 相关增量。固定入口为 N5055 编辑报告：LWG Poll 18 接入 P2019R9，LWG Poll 9 接入 P3428R4。它们是 C++29 工作草案内容，不是已发布最终标准，也不是本机标准库已经实现。

阅读前应先完成 [线程与执行方式](../../chapters/03-threads-and-execution.md)、[安全回收专题](../reclamation/README.md) 和 [执行桥接](../scheduling/03-execution-bridge.md)。本专题不替代 C10 的完整 sender 设计，也不把平台亲和性、线程命名 API 或教学 HP 实现伪装成标准库支持。

| 问题 | 正文 | 练习 |
|---|---|---|
| 创建线程前怎样给实现一个名字和栈大小建议？ | [线程属性](01-thread-attributes.md) | [F01_thread_attributes](../../exercises/F01_thread_attributes/README.md) |
| 为什么一次申请/清空多个 HP 句柄需要单独标准接口？ | [HP batch](02-hazard-pointer-batches.md) | [F02_hazard_pointer_batches](../../exercises/F02_hazard_pointer_batches/README.md) |
| 当前本机标准库的 HP、RCU、sender 到底能不能运行？ | [原生设施探测](03-native-facilities.md) | [F03_native_facilities](../../exercises/F03_native_facilities/README.md) |

三个练习都把能力探测和教学模型分开：模型只验证课程不变量；标准主体只有在头文件、特性宏、真实实例化和链接全部通过时才运行。缺能力返回 SKIP，不写成 PASS。
