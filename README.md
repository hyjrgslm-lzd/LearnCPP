# LearnCPP

面向已有 C++ 编程经验的工程师，从必要背景、标准语义和机制推导进入真实代码、实验及工程应用。正文是学习主线，练习用于验证理解；编译成功、观察程序成功和完成作业是不同的状态。

全局范围、先修和进度见 [LEARNCPP_GLOBAL_PLAN](LEARNCPP_GLOBAL_PLAN.md)，教学、实验和独立审查标准见 [CONTENT_REFACTORING_GUIDE](CONTENT_REFACTORING_GUIDE.md)。

| 课程入口 | 主讲责任 |
|---|---|
| [Engineering_Study](Engineering_Study/README.md) | C01：构建与调试、编译链接、ABI、CMake、Modules、依赖和包交付；11章/13练习，Windows验证结果见课内质量报告 |
| [Concurrency_Study](Concurrency_Study/README.md) | C08：共享状态、同步、内存模型、结构与回收；保留 CPU 性能路线作为 C13 输入 |
| [Coroutine_Study](Coroutine_Study/README.md) | C09：协程协议、控制流、帧和生命周期、I/O 与 RPC 应用 |
| [Execution_Study](Execution_Study/README.md) | C10：sender/receiver、scheduler、工作组合和运行时 |
| [Ranges_Study](Ranges_Study/README.md) | C06 的序列、视图、迭代器与惰性求值 |
| [GPU_Study](GPU_Study/README.md) | C14：设备执行、内存层级和算子实现 |
| [UELearn](UELearn/README.md) | C15 的 UE 对象、资源、任务、渲染及网络架构 |

各课独立配置和构建。先进入相应 README 的阅读路线与构建指南；其他课程族的规划入口在全局计划中，不以不存在的目录伪装成已交付课程。实际已验证、审查通过与环境受限情况，以对应课程的质量报告和证据为准。
