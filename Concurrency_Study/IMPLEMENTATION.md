# 课程重构约定与工作记录

本文件记录课程编写、集成及独立审查的工作约定。学生阅读入口为 README。

## 不变的质量要求

- 中文正文面向能阅读现代 C++、刚开始系统学习并发的工程师。
- 完整解释背景、朴素实现、问题、改进理由、协议、实验结果及适用边界。不以压缩篇幅为目标。
- 一个复杂主题可以拆成系列正文；每篇保留连续推导、可运行代码位置、最小复现命令、观察解释和自测答案。
- 每个演进版本明确标记同契约改进、场景切换或独立分支。错误示例不得参与有效性能排名。
- 每个必做 Part 都有独立 Reference；不能用占位输出或单纯退出码 0 冒充完成。
- 实现型 Starter 使用独立学生路径，空实现在线程/危险行为开始前明确失败；观察型可运行基线只证明实际检查，不能冒充全部 Part 验收。学生编辑不得污染 Reference；CTest 显式区分 student、observation、reference。
- 每种数据结构说明容量、线程角色、顺序/线性化范围、失败语义、对象所有权、元素限制及完整操作的进展保证。
- 规范以 C++26 N5050 为固定基准，C++29 另标；区分规范、教学协议、平台实现和实测结果。
- 原有主题按知识点迁移；完整性由覆盖表核验，不按目录或标题数量核验。
- 所有产出必须由未参与编写的 agent 独立 review，修改后复验；结论绑定具体文件版本。

## 目录与接口约定

- `chapters/00-execution-and-objects.md` 至 `chapters/17-diagnostics-and-sources.md` 为十八个学习入口。
- 复杂主题的连续正文放在 `topics/<topic>/`；旧根目录讲义最终转换成指向新正文的迁移入口。
- 现有练习 ID 可继续使用，README 指向新正文。新增题按知识用途命名，不维持每模块三题。
- 普通练习使用 `main.cpp` 与 `solution.cpp`。复杂项目可把被测试实现放入本题 `reference/`。
- 公共教学头仍在 `exercises/include/concurrency_study/`，命名空间 `cs`。
- 公共检查为 `cs::check(bool, std::string_view)`，Release 下仍有效，失败抛出带消息的异常；worker 中的异常须回传主线程。
- 本机默认 C++23。共用 CMake、根 README、标准索引和基准公共工具由主线程维护，专题作者不并发修改它们。
- 所有正确性检查和基准复用同一个版本的算法实现。算法变化后，消费者和所有已承诺契约重新验证。
- 单题 CMake、公共构建及能力宏由主线程集成。专题作者告知新增源文件、平台链接库及标准要求。
- 基准公共头 `concurrency_study/benchmark.hpp` 提供 `cs::bench::arguments(argc,argv)`，用 `.number("--size",默认值)` / `.text("--variant","all")` 消费参数，再 `.finish()` 拒绝未知参数。参数均是 `--name value` 对。`measure_ms(callable)` 用 steady_clock；`emit_row(suite,variant,size,threads,milliseconds,completed,details)` 在 join 后输出统一 CSV。每个基准进程运行一轮，预热/五次采样/随机顺序/外部超时由主线程的 runner 负责。作者记录实际计时范围和线程语义，不自行叠加隐藏重复次数。
- 公共 CMake 定义数值能力宏：`CS_HAS_XSIMD`、`CS_HAS_STDEXEC`、`CS_HAS_STD_SIMD`、`CS_HAS_STD_SENDERS`、`CS_HAS_PARALLEL_ALGORITHMS`、`CS_HAS_ATOMIC_MIN_MAX`、`CS_HAS_INPLACE_STOP_TOKEN`、`CS_ENABLE_UNSAFE_DEMOS`。用 `#if` 判断。缺能力的专项返回 77 表示 SKIP，仍有标准库基线的程序应检查基线并只跳过该专项。

## 学习入口

| 文件 | 内容 |
|---|---|
| 00-execution-and-objects.md | 执行与对象背景 |
| 01-concurrency-model.md | 并发心智模型 |
| 02-result-channels.md | 结果通道 |
| 03-threads-and-execution.md | 线程与执行方式 |
| 04-shared-state-and-locks.md | 共享状态与锁 |
| 05-waiting-and-channels.md | 等待与有界队列 |
| 06-cancellation-and-shutdown.md | 取消与关闭 |
| 07-coordination.md | 计数与阶段同步 |
| 08-atomics.md | 原子操作 |
| 09-memory-model.md | 内存模型 |
| 10-publication-and-lifetime.md | 发布与生命周期 |
| 11-concurrent-structures.md | 并发数据结构 |
| 12-measurement.md | 性能方法学 |
| 13-cache-and-layout.md | 缓存与布局 |
| 14-parallel-algorithms.md | 并行算法 |
| 15-simd.md | SIMD |
| 16-scheduling.md | 调度与任务组合 |
| 17-diagnostics-and-sources.md | 诊断与源码路线 |

## 实验约定

- 普通运行安全且有限结束；故意 UB/死锁的诊断程序显式开启并由外部超时机制隔离。
- 正确性运行与计时运行分开。队列逐项验证 ID/内容与契约允许的顺序；小规模历史检查补充手工协议推导。
- 一组性能比较保持契约、工作量、容量和计时边界一致；按成功完成量计算吞吐，记录批量及等待代价。
- 默认预热一次、五次独立进程采样，固定种子安排版本顺序；报告全部样本、中位数、范围及环境，不静默删异常样本。
- NUMA 实际 CPU 与页面节点分别验证；缺少硬件或放置证据时明确 SKIP。
- SIMD 分别检查数值精度与内存访问安全；明示输入域、对齐、掩码、索引和 ISA 前提。
- 回收试验明确保护成立、退休、释放/复用及最终排空条件；长读者试验限制退休数量和字节数。
- 每条技术结论注明属于推导、实测或未验证；工具不报告错误不等于证明正确。

## 当前批次

队列样章、全部主题正文/代码、独立学生路径、公共工具和构建、六篇生产源码导读均已通过非作者复审，已知阻断关闭。最终公开实验数据及质量报告亦单独通过审查；作者交付、学生完成、观察检查、Reference 与性能证据的含义分别保留。

主线程最终重新构建：离线核心 60 项中 58 PASS、2 SKIP；完整 Windows 61 项中 60 PASS、1 SKIP（含独立 fast 专项）。17组正式实验保留全部样本，365次正式进程中345 PASS、20 SKIP。原始输出、版本、独立批准和未验证边界见质量报告，不将跳过算作通过。

完整状态和实际证据见 references/quality-report.md。作者交付不等于独立复审通过。

## 本次授权的收尾顺序

1. 完成全部课程内容、代码、实验、回归及非作者独立复审。
2. 根据本次从历史勘察、Plan 规划到实施复验的流程，在仓库根目录编写 CONTENT_REFACTORING_GUIDE.md；保留完整标准和约束，以少量例子解释，不堆具体技术细节。该指引也须独立审查。
3. 仅提交本任务产生的内容和代码，保留既有 Coroutine 学习改动；推送 Git 远程并核对远端提交。
4. 前述工作全部成功后请求操作系统正常关机。发布或验证有阻断时不提前关机。用户已明确授权以上步骤。

以上为执行顺序而非预先完成声明；发布与关机的实际状态由最终交付消息确认。
