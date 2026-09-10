# LearnCPP

面向已有 C++ 编程经验的工程师，从必要背景、标准语义和机制推导进入真实代码、实验及工程应用。正文是学习主线，练习用于验证理解；编译成功、观察程序成功和完成作业是不同的状态。

## 从哪里开始

先完成 **C01 的基本构建与调试 → C02 的对象、生命周期与所有权**。C01 的 ABI、Modules 和包交付可在遇到工程问题时继续深入，不必全部学完才能进入 C02。

之后按目标选择路线：

- **序列与算法**：C02 → C06 容器/算法基础 → Ranges；自定义视图与泛型实现按需补C04。
- **并发与异步**：C02 → C08 基础同步 → C09 协程或 C10 Execution；协程与 sender 的组合放在两侧基础之后。C09 不是使用 C10 的硬先修。
- **GPU 计算**：对象与资源基础、必要数值和性能知识 → C14；无需先读完整个异步课程组。
- **引擎开发**：构建与对象基础 → C15，再按模块补齐并发、渲染和网络知识。

目录采用 `C编号_具体主题`，按全局课程版图排序。编号提供默认阅读次序，实际硬先修以章节说明为准；空号留给尚未建设的课程，不创建占位目录。C03 提供类型、不变量、错误与接口设计的公共基础；C05 补充数据表达与常用设施；C04 提供泛型、编译期编程与反射主讲；C07 等公共课程尚未完整交付，已有专题所需的先修说明和补充入口仍保留，不能把目录排序当成先修已全部齐备。

## 已有课程

| 阶段 | 课程入口 | 学习内容 |
|---|---|---|
| 公共基础 | [C01_Build_Compile_Link](C01_Build_Compile_Link/README.md) | 构建与调试、编译链接、ABI、CMake、Modules、依赖与包交付；11章/13练习 |
| 公共基础 | [C02_Objects_Lifetime_Ownership](C02_Objects_Lifetime_Ownership/README.md) | 初始化、表达式、对象模型、生命周期、所有权、智能指针、存储复用；16章/15练习单元 |
| 公共基础 | [C03_Type_Modeling_Interface_Design](C03_Type_Modeling_Interface_Design/README.md) | 类型/值/状态、错误与异常安全、多态与擦除、包装/契约、接口演进；18章、15专题练习＋综合项目与前沿单元；状态见质量报告 |
| 公共基础 | [C04_Generic_CompileTime_Reflection](C04_Generic_CompileTime_Reflection/README.md) | 泛型、查找/约束、类型计算、常量求值、反射与编译成本；25章/21单元，含进阶机制、Mp11/Hana及编译成本实验；验证与未测边界见质量报告 |
| 公共基础 | [C05_Data_Representation_Standard_Facilities](C05_Data_Representation_Standard_Facilities/README.md) | 字节/编码、解析格式化、时间/时区、路径配置、schema与资源清单包；21章/20单元，含fmt/spdlog同步前端；验证与审查见质量报告 |
| 公共基础与专题进阶 | [C06_Ranges](C06_Ranges/README.md) | 数据结构、容器、算法、Ranges与迭代器实现；保留C++26默认，标准/实现及验证状态见课程质量报告 |
| 专题进阶 | [C08_Concurrency](C08_Concurrency/README.md) | 线程、同步、原子、内存模型、并发结构与安全回收；已有 CPU 性能系列兼作 C13 入口 |
| 专题进阶 | [C09_Coroutines](C09_Coroutines/README.md) | 协程协议、控制流、帧与生命周期、取消组合、I/O 与 RPC 应用 |
| 专题进阶 | [C10_Execution](C10_Execution/README.md) | sender/receiver、scheduler、工作组合、完成通道与运行时 |
| 领域应用 | [C14_GPU](C14_GPU/README.md) | CUDA、设备执行与内存层级、GPU 性能和算子实现 |
| 领域应用 | [C15_Unreal_Engine](C15_Unreal_Engine/README.md) | Unreal Engine 对象、资源、任务、渲染及网络架构；当前覆盖 C15 中的 UE 专题 |

各课独立配置和构建。进入课内 README 的阅读路线与构建指南后，再按目标阅读正文、做练习。实际验证、独立审查和环境限制，以对应课程的质量报告为准。

全局范围、先修与进度见 [LEARNCPP_GLOBAL_PLAN](LEARNCPP_GLOBAL_PLAN.md)；教学、实验和审查标准见 [CONTENT_REFACTORING_GUIDE](CONTENT_REFACTORING_GUIDE.md)。

## 目录迁移与旧记录

2026-09-09 按课程顺序与主题统一目录名，课程内容归属保持不变：

| 旧路径 | 当前路径 |
|---|---|
| `Engineering_Study` | `C01_Build_Compile_Link` |
| `Core_Study` | `C02_Objects_Lifetime_Ownership` |
| `Ranges_Study` | `C06_Ranges` |
| `Concurrency_Study` | `C08_Concurrency` |
| `Coroutine_Study` | `C09_Coroutines` |
| `Execution_Study` | `C10_Execution` |
| `GPU_Study` | `C14_GPU` |
| `UELearn` | `C15_Unreal_Engine` |

旧日志、测量结果和冻结 SHA 清单保留当时的原文；查找其中的文件时，先按上表换算课程路径。历史 SHA 绑定当时版本，迁移中更新过路径的文件以新的验证记录为准。迁移验证与文件保留核对见[目录迁移记录](C02_Objects_Lifetime_Ownership/references/directory-migration.md)。

旧 CMake 缓存包含绝对路径，迁移后请使用新构建目录重新配置；原缓存和构建产物保留在本地，不纳入 Git。各课顶层 CMake project 与生成的 VS solution 使用课程目录名；单题 target 保留题号。Unreal 宿主项目、主模块及 Game/Editor Target 同步为 `C15_Unreal_Engine` 系列名称，原 `UELearn.uproject` 改为 `C15_Unreal_Engine.uproject`。
