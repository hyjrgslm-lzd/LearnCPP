# C07 实施规格与进度

2026-09-10。用户已批准完整方案并要求实施，随后确认复用 C08 创建的 WSL2 环境。本课以当前根全局计划的 C07/G2 与内容重构指南为规格。计划经过非作者 code-reviewer 架构复审和 critic 验收审查；预设模型不可用时用了可用原生角色，不冒称正式 OMX durable ralplan gate。

## 范围与环境

新增 C07 的正文、练习、答案、源码导读、构建和证据。根 README、全局计划以及 C08/C09/C10 README 仅补本课导航和进度。保留所有既有和并行改动，尤其 C08 当前实施与 WSL 产物；不改旧算法、学生代码、共享工具和通用指南。不提交推送，不安装系统包，不改生产系统、凭据或机器配置。

Windows 使用本机 MSVC/CMake。Linux 复用发行版 `LearnCPP-C08-Ubuntu-24.04`（VHDX 在 H:/wsl），新建本课独立临时构建、依赖、输入及证据目录，不复用 C08 build。初始只读回读：Linux 6.6.87.2-microsoft-standard-WSL2、GCC 13.3.0、CMake 3.28.3、Python 3.12.3。WSL 内部运行进程监督器；杀 wsl.exe 不能作为 guest 清理证据。

核心 C++23、默认离线。C09 现有依赖约束是 liburing>=2.15；本课独立固定 liburing-2.15，tag object `84bb497ca2f9d24ca0b9e5646fb6a05e72c0f04e`，commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`（规划期间上游 git ls-remote 回读）。仅 Linux io_uring 组合启用它，缺失时隔离构建，不改系统包或 C09。标准、库实现、kernel/opcode 能力和主体运行分开记录。

## 读者与教学依赖

读者有 C++ 编程经验，系统知识由本课从基础讲起。先修：C01 最小构建、C02 生命周期/RAII、C03 错误通道、C05 字节与路径；C04/C08 按单元需要引用。C09/C10/C11/C12 是下游，不是整课先修。

资源文件处理贯穿同步、映射和完成式 I/O；IPC、页保护、动态装载和 readiness 保持独立机制链。每条链交付连续正文、正确基线、问题与证据、推导、正反例、Student/Reference、逐 Part 解析、检查器及来源边界。不得以标题数或编译成功替代教学覆盖。

| 单元 | 主讲与责任 |
|---|---|
| L01_handles | 系统模型、系统调用与错误、独占句柄/fd、转移与释放 |
| L02_sync_io | 同步分块、短读写/EINTR/EOF、偏移、错误传播 |
| L03_virtual_memory | 地址空间、页、缺页、reserve/commit/保护、COW |
| L04_mapping | 文件映射、粒度/偏移、共享/私有、owner/view、flush 边界 |
| L05_pmr | allocator/对象生命期、pmr、对齐、资源传播、单调 arena 与定长池 |
| L06_process_ipc | 进程创建/继承/wait、pipe/共享映射及平台同步、回收 |
| L07_readiness | 非阻塞、select/poll/epoll LT/ET/ONESHOT、排空/EOF/背压 |
| L08_completion | OVERLAPPED/IOCP、io_uring、请求身份、取消和关闭排空；复杂样章 |
| L09_dynamic_loading | 装载/符号/C ABI/模块寿命与失败；桥接 C01/C18 |
| P1_file_pipeline | 同一正确性契约下 buffered/mapped/完成式资源文件处理 |
| B01_costs | 分配与 I/O 成本，计数/阶段定位、独立进程采样 |

公共接口使用 namespace c07；独占 fd/HANDLE、映射 owner/view、expected 同步结果、含 request_id/bytes/native_error 的 completion、自持 buffer 的 operation_context。默认单线程提交/回收与有限在途请求，不创建通用 Reactor 或线程池。已接受请求必须收束后才释放 buffer/context，取消请求与目标请求分开计数。共享映射仅承载明确布局字节，不承诺 std::mutex/atomic 跨进程可移植。

## 实施门槛与作者所有权

1. 父代理维护规格、公共构建/工具、导航、验证与最终版本；样章作者负责 L01/L02/L08、os.hpp 和必要正文。
2. 必要系统/句柄/同步 I/O 先修包 + 完成式取消样章，经作者实际验证和非作者审查复验后，才批量展开其他链。
3. 后续按内存/分配、进程/IPC/装载、readiness/综合项目分配完整链；共享文件由父代理集成。作者不得批准自己的产物。
4. 所有主题完成后进行跨模块集成、遮答案教学检查、技术与实验复审、修复及非作者复验。

## 验证与停止条件

Windows/Linux Debug/Release 核心与平台矩阵必须实跑；所有叶级可独立构建；Student-only 不依赖 Reference；独立 good 通过、代表 bad 精确拒绝、Student 未完成安全失败。Windows ASan 和 Linux ASan/UBSan 只覆盖安全路径，Release 检查始终生效。

测试覆盖空/尾段/块边界、溢出、无效资源、部分初始化、异常/耗尽/对齐、映射粒度、短读写/EINTR/EAGAIN/EOF、对端关闭、取消竞态与最终收束、子进程退出和清理。真实 OS 观察、可控故障注入、安全模型分别登记。进程外超时覆盖执行/退出/收尾，清理未知是真实 FAIL；教学进程禁止 daemonize/setsid 逃逸。

先正确性，再阶段计数/受控对照；性能默认 1 次预热 + 5 次独立进程采样，固定 seed 轮换版本、保存全部原始数据和指纹。Windows/WSL 分组，不清全局缓存、不预设异步/映射/池更快。flush/readback 不证明断电持久性。

WSL 缺失=环境交接 BLOCKED；库准备失败=对应路径阻断；最小探测确认 kernel/opcode 不支持才 SKIP。能力具备后主体失败=FAIL，选项 OFF 不是能力通过。完成要求覆盖及反查闭合、必需矩阵通过、阻断清零并非作者复验、导航/接线/质量报告/冻结指纹一致；环境受限的验证义务单列。

实际接线补充：guest 没有原生 pkg-config，采用 CMake 原生 find_path/find_library 接入 prepare_uring.sh 的固定源码隔离前缀，不安装系统包。C07 Linux 工作目录统一为 `/root/learncpp-c07` 下独立子目录（已用 findmnt 确认 ext4），避免与其他任务的临时清理相交。曾在第一次依赖构建后观察 `/tmp/learncpp-c07` 不再存在，原因未确证；不据此宣称 ext4 本身不持久。进程清理需要正常本机权限；受限 agent 的 taskkill Access denied 是环境失败，不能放宽真实 cleanup 检查来制造通过。

## 当前状态

- [x] 方案批准及独立规划审查；当前工作树只读勘察。
- [x] 复用 WSL 发行版与工具链回读。
- [x] 公共构建、规格、基线记录与 liburing；工具非作者复审通过，见 validation/tool-review-20260910.md。
- [x] L01/L02/L08 先修和样章：作者验证、非作者审查、修复复验。
- [x] 内存、IPC、装载、readiness 完整链。
- [x] 综合项目、源码阅读、两平台 baseline 定位及 compare 原始数据；统计独立复算通过。
- [x] 双平台完整矩阵、Student/正反检查、检测器、11 叶级及两 good 修复后的增量复验。
- [x] 最终非作者教学/证据复审、质量报告、根导航和交付指纹；入口见 quality-report.md。
