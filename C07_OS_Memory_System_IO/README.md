# C07 操作系统、内存管理与系统 I/O

为什么文件已经打开却不能随意复制句柄？为什么分配了地址空间不等于得到了物理页？为什么取消函数返回以后，缓冲区还不能销毁？本课从这些可以观察和验证的问题进入操作系统、内存与 I/O。

读者有 C++ 编程经验，尚不要求掌握系统编程。先读 [00 阅读路线](chapters/00-roadmap.md)，按需要回访 [C02 对象与资源](../C02_Objects_Lifetime_Ownership/README.md)、[C03 错误与接口](../C03_Type_Modeling_Interface_Design/README.md)及 [C05 字节与路径](../C05_Data_Representation_Standard_Facilities/README.md)。本课为 C09/C10/C11 的 I/O 应用提供基础，不要求先学完协程或 sender。

## 正文与实验

| 主线 | 正文 | 实验入口 |
|---|---|---|
| 系统、资源与同步 I/O | [系统模型](chapters/01-system-model.md)、[句柄与错误](chapters/02-handles-errors.md)、[同步文件](chapters/03-synchronous-files.md) | [L01](exercises/L01_handles/README.md)、[L02](exercises/L02_sync_io/README.md) |
| 虚拟内存与映射 | [虚拟内存](chapters/04-virtual-memory.md)、[文件映射](chapters/05-file-mapping.md) | [L03](exercises/L03_virtual_memory/README.md)、[L04](exercises/L04_mapping/README.md) |
| 分配与内存池 | [allocator 与对象](chapters/06-allocators.md)、[pmr](chapters/07-pmr.md)、[arena 与池](chapters/08-memory-pools.md) | [L05](exercises/L05_pmr/README.md) |
| 进程与 IPC | [进程](chapters/09-processes.md)、[IPC](chapters/10-ipc.md) | [L06](exercises/L06_process_ipc/README.md) |
| readiness | [非阻塞与就绪](chapters/11-readiness.md) | [L07](exercises/L07_readiness/README.md) |
| completion | [IOCP](chapters/12-windows-iocp.md)、[io_uring](chapters/13-linux-io-uring.md)、[取消和关闭](chapters/14-cancellation-shutdown.md) | [L08](exercises/L08_completion/README.md) |
| 动态装载 | [模块与符号](chapters/15-dynamic-loading.md) | [L09](exercises/L09_dynamic_loading/README.md) |
| 综合与证据 | [文件处理器](chapters/16-file-pipeline.md)、[源码阅读](chapters/17-source-reading.md)、[成本测量](chapters/18-measurement.md) | [P1](exercises/P1_file_pipeline/README.md)、[B01](exercises/B01_costs/README.md) |

正文给出背景、机制推导和解释，练习检验你能否应用这些规则。实现型 Student 的未完成状态应明确失败；运行一个观察程序并不等于完成预测、解释与扩展作业。每个练习的 Reference、独立 good、行为型 bad 和 checker 分别说明责任。

## 构建与交付证据

核心 C++23，Windows/MSVC 与 Linux/GCC 分别验证。系统机制使用各自原生 API，不能把 epoll 的就绪通知当作 IOCP 的完成通知。Linux io_uring 需要明确启用固定版本 liburing，并按实际 kernel/opcode 探测，不因头文件存在就宣称可用。

- [构建与验证](exercises/BUILD_GUIDE.md)：整课、单题、Student-only、检测器和 WSL 入口。
- [实施规格](references/implementation-spec.md)、[知识覆盖与下游反查](references/coverage.md)、[规范与实现索引](references/standards-and-implementations.md)。
- [质量报告](references/quality-report.md)：约定本机验证、独立审查、版本对应和未验证边界。
- [成本分析与原始数据](references/measurements/cost-analysis.md)：两平台 220 个正式样本及阶段归因；[交付指纹](references/delivery-manifest-r2.json)列出逐文件 SHA256。

本课实验只操作自有临时资源，不运行无界负载或自动修改机器配置。进程外超时是兜底机制，超时及清理失败是真实失败，不是正常取消结果。
