# NUMA / 调度专题：作者验证与集成交接

本记录是独立 review 五项阻断修复后的作者复测，日期：2026-09-08。修订已完成并冻结 C++ 源码与题型；仍需由原 reviewer/Lorentz 按下列最终文件哈希复验，没有预先标记独立验收完成。当前会话没有可调用的原 reviewer 通道，由主线程安排复验。

## 文件范围与接线

本批作者修改/新增 27 个文件，均在 `Concurrency_Study` 内：

- `chapters/16-scheduling.md`。
- `topics/scheduling/01-static-dynamic.md`、`02-work-stealing.md`、`03-execution-bridge.md`、本记录 `verification.md`。
- `topics/numa/01-topology.md`、`02-affinity.md`、`03-placement.md`、`04-sharding.md`。
- `exercises/J3_numa_concept/main.cpp`、`solution.cpp`、`README.md`。
- `exercises/M1_work_stealing_pool/main.cpp`、`solution.cpp`、`README.md`。
- `exercises/M2_execution_bridge/main.cpp`、`solution.cpp`、`README.md`。
- `exercises/N1_numa_placement/main.cpp`、`solution.cpp`、`README.md`。
- `exercises/include/concurrency_study/numa.hpp`、`work_stealing_pool.hpp`。
- `exercises/runtime_tests/scheduling_test.cpp`。
- `exercises/benchmarks/numa_bench.cpp`、`scheduling_bench.cpp`。
- 旧根 `16-模块M-工作窃取与结构化并发桥接.md` 用内容更新方式改为迁移入口。

没有编辑任何 CMake、公共 benchmark/runner、根 README、标准/质量/覆盖索引、旧 13、chapter17 或其他作者文件。未提交 git。各题仍为单翻译单元，无定制多源要求。Windows 的 J3/N1 main/reference 和 numa_bench 需要 Psapi，主线程已接入并由 J3/N1 叶项目实际链接验证；无需新增 NUMA runtime target。Linux 使用内核 syscall，不增加 libnuma 链接依赖。

最终四题分类均为 OBSERVATION。M1/M2 是独立调度 baseline/值管线，J3/N1 是平台诊断探针；main 与 README 明确说明成功只覆盖实际运行的检查，不完成全部 Part。四题都没有 main include solution.cpp。主线程已经将 M1/M2 CMake 同步为 OBSERVATION，作者已读取确认，不再改题型。不把中间快照用于最终分类或漏测结论。

## 五项阻断与修复证据

| 独立 review 问题 | 最终修复 | 作者复测 / 尚未实测 |
|---|---|---|
| Windows 对已 reserve 地址 commit 会忽略首选 node | 每基础页以 nullptr 地址新建 RESERVE\|COMMIT；五个 variant 统一独立页指针布局；逐块析构、部分构造失败释放已有块 | 本地新布局的 CPU/全部页/内容检查通过；多节点首选分布未实测 |
| producer 创建失败时 errors/seen 提前销毁 | seen/errors 先于 pool，producers 最后；反向收束为 producer join→pool 排空→数据销毁 | 组合注入第二 producer 启动失败及活 producer 的真实 submit 分配 bad_alloc；原异常、producer 错误、accepted task 排空均通过；Release 和 MSVC AddressSanitizer 通过 |
| Linux interleave 次序/零相位假设错误 | 节点集合排序、按完整逐页循环接受固定相位；确认一次后所有后续快照必须保持同一序列；要求 OS 基础页、MADV_NOHUGEPAGE | 双节点奇相位、反向请求加奇相位、三节点顺序、错误逐页排列和未驻留反例模型通过；Linux API/实际页放置未实测 |
| 单 GROUP_AFFINITY 无法恢复 Win11 默认跨组 affinity | 移除通用 affinity_guard；唯一 setter 位于 on_cpus 新建专用线程内，永久绑定后退出；不修改调用者或复用 worker | J3 正常/异常路径均检查不在调用者线程执行，CPU 端点和异常回传通过；多组机器未实测，不承诺完整恢复 |
| 两 memory node 但一个可用 reader 导致 shared Part 丢 SKIP | 六个 Part 分别记录 PASS/SKIP/FAIL；FAIL 优先为1，否则任一 SKIP 为77；继续其他可跑 Part | 模拟“两内存节点、一 CPU node”并合并后续成功，仍为77；加入 FAIL 后为1；本机实测3 PASS/3 SKIP |

Windows 分配修复依据 [VirtualAllocExNuma 参数说明](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualallocexnuma)，亲和性边界依据 [SetThreadGroupAffinity 的 Win11 跨组说明](https://learn.microsoft.com/en-us/windows/win32/api/processtopologyapi/nf-processtopologyapi-setthreadgroupaffinity)，Linux 相位模型依据[内核内存策略文档](https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html)。旧逐页 commit 实现的 API 错误已修复，不再将它解释为环境 SKIP。

## 环境和构建

Windows 10.0.26220、SDK 10.0.26100、MSVC 19.51.36256.0、CMake 4.2.3、x64、Release。J3 实测 primary group 0 候选范围内32个逻辑 CPU、16组 SMT 核心关系、一个 package/NUMA node 0、4096字节基础页；新布局每基础页一个 reservation，allocation granularity=65536。候选 primary group 不代表原线程已被限制单组，不代表完整跨组 affinity。

作者独立构建目录位于 `exercises/build/scheduling-author`、`bridge-author`、`numa-author`、`placement-author`。共享 `full-windows` 仅作为固定 stdexec 源码依赖读取，没有向共享构建运行配置。沙箱内 MSBuild 因 PATH/Path 重复报 MSB6001，已通过获准的沙箱外独立构建完成验证；没有修改全局环境或系统配置。

CMake 默认要求 C++23，但本机生成的 MSVC project 将该要求映射为 `/std:c++latest`。为排除隐藏标准依赖，另以 `/std:c++23preview /O2 /DNDEBUG` 显式编译 M1/M2 Reference、N1 main/reference、runtime 和两个 benchmark。M1/M2 显式 C++23 Reference 已实际运行通过。M2 使用固定 commit `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`，真实依赖编译通过；/W4 下依赖自身出现对齐与局部名遮蔽警告，课程代码没有编译错误。

## 正确性运行

| 对象 | 结果 | 证据范围 |
|---|---|---|
| M1 main / Reference | PASS | 三种调度逐 ID、零 worker 拒绝、单/多 worker 递归、move-only callable、异常传递、关闭排空、重复 join、关闭后拒绝、确定发生窃取 |
| scheduling runtime | PASS | 20 轮、每轮 400 ID、四提交者逐项唯一性；跨池身份；submit/shutdown 竞争 |
| M2 main / Reference | PASS | 42、123、error、stopped、停止绕过 then、when_all、upon_stopped 恢复 |
| M2 `CS_HAS_STDEXEC=0` Reference | 77 SKIP | 无依赖头仍可编译，明确提示缺固定依赖 |
| J3 main / Reference | PASS | CPU 列表解析；允许集合；至多四个 CPU before/after 匹配请求 |
| N1 main | PASS | 16 页本地请求、CPU 与全部页面验证、扫描总和 |
| N1 Reference | 77 SKIP，部分已验证 | 32 页共享读取本地检查；128 页 local/firsttouch/parallel-init 全部通过；单节点缺 remote/interleaved 与跨节点 reader |
| 非法基准参数 | 1 | scheduling variant=all 被拒绝；NUMA size=1 非整页被拒绝，均不输出有效 CSV 行 |

上述 M1/M2 main 的 PASS 仅指 OBSERVATION 实际覆盖项；全部结果通道/关闭/递归等结论来自各自独立 Reference。最新四个 main 已经重新编译运行，均以 OBSERVATION 开头，分别检查三调度小负载、值管线42、单 CPU 探针、本地16页探针。M2 缺依赖分支的77记录来自此前相同能力分支检查，不把原生标准 sender 视为可用。

scheduling runtime 另以 `/std:c++23preview /O1 /Zi /DNDEBUG /fsanitize=address` 构建为 `exercises/build/scheduling-author/scheduling_asan.exe` 并运行通过。该程序内真实 submit 的 make_shared 分配由本线程单次 operator new 注入为 bad_alloc；producer 创建故障是在启动位置确定性注入的 system_error。检查原始创建异常未被 cleanup 替换、活 producer 的错误保存在仍存活的 errors 中、accepted task 在 seen 析构前完成。ASan 未报告悬空访问；这不证明所有交错或真实 OS 耗尽路径正确。

N1 叶项目 CTest 把 77 正确归类为 Skipped。最新输出为 `passed=3 skipped=3 failed=0 exit=77`：shared-read 缺跨节点 reader，remote/interleaved 缺另一 memory node；local/firsttouch/parallel-init 均通过。不能把 CTest 的“0 failed”写成“所有 NUMA 条件已验证”。日志显示新布局128页最初均未驻留，写入后、预读后及扫描后均为 node 0；双 reader 实际 CPU 为 `0:0` 和 `0:2`。

## 公共 runner 单轮进程采样

两个基准均由公共 `exercises/tools/run_benchmarks.py` 驱动：固定种子 42、预热 1 次、测量 5 次、外部超时 30 秒。每个进程仅输出选中 variant 的同名统一 CSV。报告保留全部样本与每轮 stderr，不要求特定加速。

调度本轮报告：run.json（本机归档 `exercises/build/scheduling-author/review-fixes-01/run.json`），状态 PASS。参数 `--size 1024 --threads 4`；每轮 completed=1024。前置检查同时运行 M1 Reference 和包含组合故障注入的 scheduling runtime。

| variant | 全部正式样本 ms | 中位数 ms | 范围 ms |
|---|---|---|---|
| static | 2.9823, 3.0532, 3.1132, 2.9845, 2.9443 | 2.9845 | 2.9443–3.1132 |
| dynamic | 1.3650, 1.2349, 1.3322, 1.3917, 1.4163 | 1.3650 | 1.2349–1.4163 |
| stealing | 1.6147, 2.2030, 2.2582, 1.9954, 1.6025 | 1.9954 | 1.6025–2.2582 |

这些值只描述本次小负载及完整调度实现；不证明 stealing 普遍比 dynamic 慢。计时包含分配、创建执行资源、调度、计算及 join。

NUMA 本轮报告：run.json（本机归档 `exercises/build/numa-author/review-fixes-01/run.json`），状态 PARTIAL_SKIP、allow_partial=false。参数 `--size 1048576`，每有效样本 completed=131072 words。五版均为新的独立基础页布局：256个 reservation、65536字节分配粒度、4096字节页。runner 前置检查使用 J3 Reference 和 N1 本地探针；完整 N1 Reference 已单独运行并如实报告77，没有伪装成全能力 PASS。

| variant | reader 数 | 全部正式样本 ms | 中位数 ms | 范围 ms |
|---|---|---|---|---|
| local | 1 | 0.1456, 0.1437, 0.1623, 0.1702, 0.1455 | 0.1456 | 0.1437–0.1702 |
| firsttouch | 2 | 0.1342, 0.1788, 0.3418, 0.2128, 2.4943 | 0.2128 | 0.1342–2.4943 |
| parallel-init | 2 | 0.1904, 0.1498, 0.1843, 0.1457, 0.1676 | 0.1676 | 0.1457–0.1904 |
| remote / interleaved | — | 各六轮均 77，无有效 CSV | — | — |

保留 firsttouch 的2.4943ms样本，没有静默删除异常值。单节点上没有远端性能结论；local 的线程数又不同，不能把这些行统称为同一并行度的速度排名。计时包含 reader 创建、专用线程绑定、页指针查找/页内扫描和 join，排除分配、初始触页、预读和页查询；未采集区间内 fault/迁移事件。

新 runner 的原生退出契约已端到端检查：local+remote、4096字节、预热1次/正式1次的默认报告（本机归档 `exercises/build/numa-author/contract-default-01/run.json`）为 PARTIAL_SKIP，父进程直接取得 native returncode=77；同条件增加 `--allow-partial` 的显式允许报告（本机归档 `exercises/build/numa-author/contract-allow-01/run.json`）仍标 PARTIAL_SKIP，但 native returncode=0。这两轮只验证退出契约，不纳入五样本性能表。PowerShell 对裸非零命令可能只向外报告1，因此没有用外层 shell 码替代子进程实际码。threads=0 约定为未测量，本组均已明确 reader 数，不使用0。

首版报告仍保留：旧调度样本（本机归档 `exercises/build/scheduling-author/results-01/run.json`）、旧 NUMA 样本（本机归档 `exercises/build/numa-author/results-01/run.json`）。它们是历史快照，旧 NUMA 连续布局不能充当修订后分块布局的性能证据。每份 runner 报告保存采样时的源码/可执行文件指纹；本节随后补写的文档和 OBSERVATION 文案不冒充采样时的全课程快照。

## 未验证与明确边界

- 没有多节点硬件，remote、interleaved 和跨节点共享 reader 只实现并编译，未取得实测页面证据。
- Linux 源码分支已实现 sched affinity、sysfs/allowed memnodes、mmap/mbind/move_pages；本机 WSL 查询提示未安装子系统，未编译或运行 Linux，不安装系统组件来制造“已验证”。
- Windows 多 processor group 完整拓扑与复用线程 affinity 恢复不在本实现范围。discover 仅给出 primary group 候选子集，setter 只在随后新建的专用线程内运行；没有多组实测，绝不由旧单组结构推断完整恢复。
- 已注入 producer 启动失败与真实 submit 分配失败的组合；没有注入 pool 构造内部线程创建失败，也没有让 OS 实际耗尽线程资源。后者仍由关闭/通知/join 推导，不声称穷尽所有交错。
- 两次 CPU/页观测只是端点证据，不是连续迁移轨迹。没有页锁定、硬件计数器或“零 page fault”保证。
- M1 无 NUMA 感知 victim 选择，无锁 Chase–Lev 未实现；跨池 worker wait、任意重入/资源依赖图不受支持。按节点分片是 N1 的独立场景分支。
- 原生标准 sender 本机能力检测未支持；M2 验证的是固定 stdexec，不把库实现通过写成标准库 C++26 全支持。

## 代码版本指纹

以下 SHA-256 对应本记录交接的代码文件，便于独立 reviewer 绑定版本。后续修改需要更新检查和记录；构建产物与每次进程的哈希另存于 runner 报告。

| 文件（相对 exercises） | SHA-256 |
|---|---|
| include/concurrency_study/work_stealing_pool.hpp | E825D39C85D7A3AC222ABD830F81DAC72D5715751D18D6AC56C3CBAEA2B63994 |
| include/concurrency_study/numa.hpp | 2C5F3D915AB0747CA9C2C0CAF078B381AD681B348725CF4461E85A62A15FEB1B |
| M1_work_stealing_pool/main.cpp | FA8D1B374805BA722428ED65FB9E8F600CE85215D365E961F263914BF471532C |
| M1_work_stealing_pool/solution.cpp | 3644C69254DCCD22FCB7D51BAD553FBA6E02B58FC032B961717C8044E4F4AED6 |
| M2_execution_bridge/solution.cpp | 62BDB24D0441EB2DF6F347FE09585740338E81ECBD256563C66751964F35C610 |
| M2_execution_bridge/main.cpp | D98B7E889E275F7F4D556E1E8D25297FC63950DADBF822B077FD36D1A97B7CBE |
| J3_numa_concept/main.cpp | 2E4AA596FD2801FA444E15BFF624E2DA7388B316DD8DB1294459B7FEB698DB2F |
| J3_numa_concept/solution.cpp | CE7844F5B4CCE58773B2F7B50F79D572601CAECC599E6C7FC275B16A488CB6BE |
| N1_numa_placement/main.cpp | 837C646DEC093AC8FC436828041547005C28CECCE908A3F2EA0D74779A36E493 |
| N1_numa_placement/solution.cpp | 2F8FAF49A2ECA7215CC6C532B0734F0790360820FC8D514E86B9CB0EE27016F4 |
| runtime_tests/scheduling_test.cpp | 4230EA174837738B57A1646CEF0A8EA384F852C8CE96AB2958F0694628F784F7 |
| benchmarks/scheduling_bench.cpp | 572050F67B76807606BAFEAB72D761D45D1DE661CB9C9D0885C1CC541D617142 |
| benchmarks/numa_bench.cpp | 6DD95D0DDDB1E0C8A860E2D2A4FEF1DEAED7E92669127EE18CC981B58889AC7C |
