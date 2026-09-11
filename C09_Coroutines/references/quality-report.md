# C09 质量报告

更新日期：2026-09-11。当前状态：**实现与本机验证完成**。37 单元覆盖、独立审查、Student/Reference 分离及最终编译输入指纹均记录在下述证据中；受限环境单列。

## 范围与交付口径

依据[实施规格](implementation-spec.md)，37 单元全审计、按缺口增量修改。[覆盖表](coverage.md)登记每题入口、处置与下游反查；学生 TODO 保留，观察程序通过、学生起点拒绝、Reference 通过和独立 good/bad 分开。没有提交、推送、系统安装或 CI 增补；并行 C10 修改不属于本课。

正文沿执行背景、使用、语言机制、真实 I/O、RPC 和 mini 库推进。复杂样章先经非作者审查后才展开其余批次；[S0 规格](validation/c09-refresh/reviews/s0-spec-review.md)、[S1 样章](validation/c09-refresh/reviews/s1-review.md)记录对应版本。

## 修复与原证据

| 问题 | 根因与处理 | 证据 |
|---|---|---|
| task 结果移动异常泄漏 | 提取期间保持既有 RAII owner；异常仍传播，帧必释放 | [S1](validation/c09-refresh/s1/)，原 alive=1、修复后 alive=0 |
| scope 启动失败后不能排空 | 预留 in_flight 后创建失败没有回滚；现在回滚并通知 | [scope](validation/c09-refresh/scope/)，原 in_flight=1/starts=0 后超时 |
| sync_wait 栈状态通知窗口 | 持锁完成 done 与 notify，等待方不能提前销毁 cv | [runtime 审查](validation/c09-refresh/reviews/runtime-review.md)；静态生命周期推导，不宣称实测 CV 崩溃 |
| shared_task 等待者提前销毁 UAF | 弱登记、phase、注销和逐个恢复；producer 按值保活，消除引用环 | [runtime 审查](validation/c09-refresh/reviews/runtime-review.md)，原 ASan UAF、修复复验通过 |
| 爬虫错误丢失 URL | 移动请求后 catch 读取已移动字段；保留移动前 URL | [基础审计](validation/c09-refresh/authors/foundation/audit.md)，故障注入前后对照 |
| H1/H2/H3 常量蒙混 | 将学生操作与检查体分开，变化输入，真实操作 trace、私有检查状态、互斥且恰好一次完成；独立正反例已复验 | [教学/实验审查](validation/c09-refresh/reviews/content-experiment-review.md) |
| RPC 学生入口与答案契约不同 | 统一 8 字节长度头/4096/QCR，公开接口、逐阶段安全 stub 与同一 semantic checker | [RPC 审查](validation/c09-refresh/reviews/rpc-review.md) |
| 测试自身捕获型 coroutine lambda UAF | bridge/run_loop 的临时闭包先死；改为命名协程、参数显式保活 | [额外诊断](validation/c09-refresh/final/diagnostics-extra/)；原 stack-use-after-scope，修复后作者与非作者复验均通过 |
| GCC 无效 dump 参数 | 正常目标不再强制 -fdump-tree-coro；显式诊断改为实际支持的 pass/tree dump | [编译器诊断](validation/c09-refresh/compiler-diagnostics/) |

when_any 的一次审查曾建议首 error 即 cancel，已撤回。该课使用 first-success：成功 winner 才请求停止；没有成功时输入需要有限完成或外部取消。保留原 timeout 与 error-first/later-success 对照，不改变契约来制造通过。

## 验证矩阵

最终冻结矩阵（其后只清理 H2 CMake 文件末尾空行，另有文档/证据更新）：

| 路线 | 阶段结果 | 原记录 |
|---|---|---|
| Windows Release/Debug | 各 64 PASS、1 原生 task probe SKIP | [Windows](validation/c09-refresh/final/windows-20260911T050301Z/summary.json) |
| Student Reference=OFF | 30 项中 7 提供实现/观察通过、23 起点明确拒绝 | [Student](validation/c09-refresh/final/student-20260911T050302Z/summary.json) |
| WSL Release | 50 PASS，四个 generator 相关目标由独立能力探针排除 | [Linux](validation/c09-refresh/final/linux-20260911T050306Z/summary.json) |
| ASan/UBSan/LSan | 16 个主体通过：原 13 项，加修复后的 run_loop/bridge 及 RPC | [核心诊断](validation/c09-refresh/final/diagnostics-20260911T034829Z/summary.json)、[补充诊断](validation/c09-refresh/final/diagnostics-extra/) |
| TSan | 干净能力探针 exit 66：unexpected memory mapping；本环境 SKIP | [能力探针](validation/c09-refresh/final/diagnostics-20260911T034829Z/thread-capability-run.json) |
| 独立 mini good/bad | when_any/shared_task/executor 与 task sender 同一检查器正反例通过；默认作业仍未实现 | [mini Student 审查](validation/c09-refresh/reviews/mini-student-review.md) |
| RPC | Student 真实 FAIL；答版、独立 protocol good、精确 bad 拒绝、Reference 分开通过 | [RPC 审查](validation/c09-refresh/reviews/rpc-review.md) |

Student 隔离检查读取 CMake 活动源文件和编译器依赖记录：51 个课程源文件，33 份源报告、21 份依赖日志，无缺失源文件、无 Reference 输入；[隔离结果](validation/c09-refresh/final/student-20260911T050302Z/isolation.json)。7 项通过仅证明当前提供的实现/观察，不能推断整题所有 Part 完成。

使用 MSVC 19.51 / VS 18 2026、GCC 13.3 与已有 Clang 18。liburing 2.15 的真实 queue_init 成功，I2/WSL 已实测。固定依赖 SHA 由本课离线配置脚本回读；实际命令、编译选项、源文件和进程结果保存在各记录中。

## 性能与能力边界

F3 使用同契约 A/B、单调时钟、每版 1 预热和 5 独立进程样本；计数与无插桩诊断分开。Clang 18.1.3 的两条实际 consumer 路径都没有动态分配，均化简为求和循环；独立导出的 range_values 仍分配 48 字节。没有 elide remark。旧作者报告把全模块存在分配调用解释为“未观察 HALO”，现按真实调用路径更正。

本轮 n=32 的 local/escaped median 为 2.660/2.237 ns，n=256 为 34.714/26.161 ns。两版循环代码不同，不能把耗时差归因于只有一版 HALO。Windows 前后快照存在其他构建进程，干扰未排除；不做普适排名或倍率结论。见[完整实验判读、原始样本与 IR/汇编](validation/c09-refresh/performance/final/analysis.md)。

- 本机标准库缺 std::execution::task，原生 probe 与 stdexec 参考实现分列。
- GCC 13 缺 generator/print，只影响相应能力；不是整个 Linux 课程通过或失败的替代说明。
- Folly/Cobalt 保留正文、练习、Reference 与步骤，当前未构建运行。
- G1 保留单线程有限教学基线，waiter owner 必须活到 producer 完成；提前销毁 waiter 的 ASan 失败已保留，不声称 G1 支持此操作。mini_ref shared_task 支持已排序的等待者放弃；同一帧 destroy 与 resume 仍需调用方序列化。run_loop 中的排队 handle 不拥有帧，owner 必须保持任务直到队列消费/排空。
- RPC good/ 是 public API 对应的 Reference-adapted 答版，不能称独立 good；独立 protocol good 只证明协议 Part。
- 测试和工具无报告不穷尽全部交错、平台与输入。所有异常、超时或清理失败保留为真实失败；受限能力单列。

## 独立复核

[runtime 审查](validation/c09-refresh/reviews/runtime-review.md)、[H 与实验审查](validation/c09-refresh/reviews/content-experiment-review.md)、[mini Student 审查](validation/c09-refresh/reviews/mini-student-review.md)、[RPC 审查](validation/c09-refresh/reviews/rpc-review.md)分别保留审查对象、发现、修复复验及 APPROVE/COMMENT 边界。最终文件一致性与证据回读见[集成验收](validation/c09-refresh/reviews/final-integration-review.md)。逐文件改动说明见[文件明细](changes.md)。

## 复现与版本

从仓库根目录使用[构建指南](../exercises/BUILD_GUIDE.md)中的 run_matrix.py；诊断在已有 WSL 运行 run_diagnostics.py。单题优先 CMake target 与 CTest 名称。三套最终矩阵各 196 个编译相关输入中，195 个与当前 SHA-256 一致；H2 CMake 文件仅去掉末尾空行，先前所有内容未变，见[格式变更前后指纹](validation/c09-refresh/final/post-matrix-formatting.json)。随后 Windows RefON/RefOFF 配置均通过，见[RefON](validation/c09-refresh/final/post-format-configure-windows.json)、[RefOFF](validation/c09-refresh/final/post-format-configure-student.json)及[源文件回读](validation/c09-refresh/final/final-source-readback.json)。源码/文档/数据最终指纹由 [delivery manifest](validation/c09-refresh/delivery-manifest.json)记录，导航/37 单元由 [audit](validation/c09-refresh/delivery-audit.json)检查。

[旧质量报告原件](validation/c09-refresh/history-quality-report-20260908.txt)和旧 validation 保留。历史 Coroutine_Study 路径只作历史证据；当前构建使用 C09_Coroutines 的新目录。编译产物和本机可执行副本不进入交付清单。部分工具本地化输出由公共监督器按 UTF-8 解码，测试的英文诊断、退出码、超时与清理状态另有结构化字段。
