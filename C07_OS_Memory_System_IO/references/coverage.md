# C07 知识覆盖与下游反查

每行同时追踪主讲、可执行入口和下游问题。构建/导航检查不证明教学深度；实际矩阵、独立审查、标准行为差异及版本对应由质量报告链接到具体证据，不能因文件出现就判为通过。

| 知识与核心问题 | 主讲与解析 | 代码/实验 | 下游反查 |
|---|---|---|---|
| 进程/线程、系统调用、用户态/内核态、阻塞与调度 | [系统模型](../chapters/01-system-model.md)、[进程](../chapters/09-processes.md) | [L01](../exercises/L01_handles/README.md)、[L06](../exercises/L06_process_ipc/README.md) | C08 线程生命期，C11 阻塞服务为什么占用执行线程 |
| fd/HANDLE 的复制、继承、释放与错误捕获 | [句柄](../chapters/02-handles-errors.md) | [L01](../exercises/L01_handles/README.md) | P1 文件/映射/请求谁先关闭；C18 模块责任 |
| 短读写、EINTR、EOF、偏移与缓存 | [同步 I/O](../chapters/03-synchronous-files.md) | [L02](../exercises/L02_sync_io/README.md) | C11 分帧，C12 文件写入；不能把一次 read 当整条消息 |
| 记录锁/字节范围、显式偏移与锁所有者 | [同步 I/O](../chapters/03-synchronous-files.md)、[IPC](../chapters/10-ipc.md) | [L06 文件锁观察](../exercises/L06_process_ipc/README.md) | C12 并发文件访问；Linux 进程关联锁不能用同进程两个 fd 伪测 |
| 地址空间、页、缺页、保护、COW、reserve/commit | [虚拟内存](../chapters/04-virtual-memory.md) | [L03](../exercises/L03_virtual_memory/README.md) | C08 NUMA/C13 内存观察；页可访问与对象合法分层 |
| 映射粒度、偏移、共享/私有、owner/view | [文件映射](../chapters/05-file-mapping.md) | [L04](../exercises/L04_mapping/README.md) | P1 mapped 后端，C12 文件可见性与持久性 |
| allocator、构造/销毁、对齐与失败 | [allocator](../chapters/06-allocators.md) | [L05](../exercises/L05_pmr/README.md) | C06 容器存储与对象、C09 帧分配 |
| pmr 的资源寿命、传播、嵌套容器 | [pmr](../chapters/07-pmr.md) | [L05](../exercises/L05_pmr/README.md) | P1 批次缓冲，容器不能越过 upstream 资源寿命 |
| arena、定长池、复用、容量与异常 | [内存池](../chapters/08-memory-pools.md) | [L05](../exercises/L05_pmr/README.md)、[B01](../exercises/B01_costs/README.md) | C15 专有 allocator 的通用先修；并发池另需 C08 |
| 进程创建、继承、wait/退出与回收 | [进程](../chapters/09-processes.md) | [L06](../exercises/L06_process_ipc/README.md) | 自启实验进程正常/超时收尾，不以杀 wsl.exe 代替 |
| IPC、字节布局、共享映射与跨进程同步 | [IPC](../chapters/10-ipc.md) | [L06](../exercises/L06_process_ipc/README.md) | C11 RPC 之前的传输/生命期；共享 mutex 非可移植前提 |
| 非阻塞、readiness、LT/ET/oneshot、背压 | [readiness](../chapters/11-readiness.md) | [L07](../exercises/L07_readiness/README.md) | C11 Reactor；事件不是已完成的读操作 |
| OVERLAPPED/IOCP、提交与完成的分离 | [IOCP](../chapters/12-windows-iocp.md) | [L08](../exercises/L08_completion/README.md) | C09 awaiter / C10 operation_state 的存活责任 |
| io_uring、SQ/CQ、能力与实际提交/完成 | [io_uring](../chapters/13-linux-io-uring.md) | [L08](../exercises/L08_completion/README.md) | C09/C10 Linux 完成源；liburing 源码索引 |
| 取消竞态、请求身份与关闭排空 | [取消与关闭](../chapters/14-cancellation-shutdown.md) | [L08](../exercises/L08_completion/README.md)、[P1](../exercises/P1_file_pipeline/README.md) | C09/C10/C11 的取消不等于请求停止占用缓冲 |
| 动态装载、符号、C ABI、模块卸载 | [动态装载](../chapters/15-dynamic-loading.md) | [L09](../exercises/L09_dynamic_loading/README.md) | C01 链接/ABI 回访，C18 插件/FFI 后续 |
| 综合契约与可归因成本 | [P1](../chapters/16-file-pipeline.md)、[源码](../chapters/17-source-reading.md)、[测量](../chapters/18-measurement.md) | [P1](../exercises/P1_file_pipeline/README.md)、[B01](../exercises/B01_costs/README.md) | C13 性能归因、C12 持久性边界；不跨平台排名 |

完成状态与原始记录统一见[质量报告](quality-report.md)。规范与实际接口来源见[标准和实现索引](standards-and-implementations.md)；逐 Part 的学生编辑位置、Reference、good/bad 和解析在对应题目，不在本表复制另一份状态。
