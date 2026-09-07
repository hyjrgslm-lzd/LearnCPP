# F 批复审修复与交接记录

日期：2026-09-08。此记录替换首轮作者结论，回应原非作者 review 的四项 P2、Darwin 两项 P2及 Faraday 的教学路径/命令收尾。下面是作者实测与证据交接，不等于原 reviewer 的修后独立复验通过；需由同一非作者按末尾版本哈希复验。

本批次未修改共享 CMake、benchmark.hpp、根 README、全局质量索引、J3/M 或主线程 build/verify-core、full-windows 目录，未提交 git。原始样本没有删除。主线程报告同阶段存在集成重建，无法证明整机空闲，作者数据统一标为**环境未空闲的开发观察**；正式组由主线程在源码稳定后独立运行，作者不再追加测量。

## 六项 P2 的修复与检查

| 问题 | 当前修复 | 作者验证 |
|---|---|---|
| 已有正确输出掩盖漏写 | L2 每轮重填 inclusive/exclusive 和仿射输出；K1 aligned/masked-tail 重填全有效区；numeric_test 加法、条件、置换、GEMM 重填错误哨兵，保留边界 canary | 严格与 ASan 通过；分别跳过 inclusive、exclusive、仿射写入及 masked-tail/GEMM no-op，检查器均拒绝 |
| fast 编译侵蚀 oracle | strict checker 与 fast kernel 独立 TU，只共享无算术定义的 API；/GL-，链接 /LTCG:OFF /OPT:NOICF；numeric_test 禁止 fast 编译 | 分离单元及 ASan 通过；补偿保留抵消中的 1，比较拒绝 NaN/Inf/越界；两单元错配编译选项均被宏守卫拒绝 |
| 空任务线程字段错误 | threaded/reduce_threaded 空输入报告 threads=1、workers=0、caller=1；非空报告裁剪后的 worker 数，caller 另记；标准库 0 仅表示 backend unmeasured | Cap3 Reference 解析实际 CSV，检查 0/1/3 项的两种手写版本；公共 runner 的空任务五样本均通过 |
| 轻/重与跨规模迁移缺失 | light=x*x+2；heavy=double 递推 96 阶几何多项式；独立闭式及截断界检查，正文完整推导 | 两负载、0/1/7/65/257 项及所有可用策略、定义域拒绝、skip-write 检查通过；三档六组共 150 个样本完整保留 |
| 自动程序无进程外超时 | verify-numeric.ps1 的 Reference/观察/Starter/fast/driver 执行全部复用公共 run_process，默认 30 秒；不另造进程管理 | 0、77、7 原始退出码如实；执行中和退出回调中的受控挂起均 TIMEOUT，不能被误当预期 Starter exit=1 |
| leaf 与公共 bench preset 目录冲突 | SIMD 文档统一使用独占 build/simd-leaf-bench，exe 路径同步 | 实际 CMake 配置、Release 构建、小输入驱动通过；CMAKE_HOME_DIRECTORY 指向 exercises/benchmarks |

skip-write 故障只对非空输出有判别力；空任务本来无需写。哨兵检测写，ASan 和范围推导共同检查访存边界，不能把一次工具通过等同于完整证明。

原 `/fp:fast + CS_NUMERIC_FINITE_ONLY` 整文件测试方式已撤回：其中 Neumaier 和误差比较也受 fast 影响，首轮“通过”不得继续作为可靠 oracle 证据。现改用下述独立严格判分单元，常规 numeric_test 不再通过跳过特殊值来伪装成 fast 专项。

## 教学角色和实际入口

- **OBSERVATION**：J1、J2、K1、K2、K3、L1、L2 的 main 保留给定基线、预测和检查。源码/README 明确其退出成功只覆盖实际列出的检查，不自动判定所有 Part 完成。
- **IMPLEMENTATION**：L3 和 Cap3 的 main 有真正待填的 student_* 路径，检查实际学生输出。当前默认 exit=1，不能算 Reference 通过，也不能将超时 exit=1混作正常未完成。
- 两题各有独立 benchmark.cpp。正式目标为 **L3_par_vs_seq_bench_benchmark**、**Capstone3_parallel_compute_benchmark**；测量不再使用 main。
- 修后复审发现 Cap3 默认学生检查还隐含要求“选做 SIMD”。主线程已将其改为显式 `--with-simd`：默认仅必做项，选做先通过必做项再检查能力及 SIMD 输出；对应 README 和本表版本已同步，待原非作者独立复验该增量。
- L3 README 分开“实现 Part 1–3”和“实验任务 A–D”；Cap3 分开实现 Part 与实验任务。两份 README 都给出了完成顺序、开启 TEST_STARTERS/BUILD_BENCHMARKS 的完整命令、student/reference 过滤检查、目标构建、$exe/$check 实际赋值和完整 runner 流程。
- 已将原范围内的 NUMA 迁移链接接到独立作者实际提供的 topics/numa/01-topology.md，没有修改 NUMA 实现。

## 源文件和 CMake 接线

普通算法仍在 exercises/include/concurrency_study/numeric_kernels.hpp 和 simd_kernels.hpp，Reference 与驱动复用它们。主线程已经正式接入两题 benchmark.cpp 的单源目标；无额外依赖或链接库。

快数学专项新增：
- [fast_math_api.hpp](../simd/fast_math_api.hpp)：仅声明。
- [fast_math_check.cpp](../simd/fast_math_check.cpp)：严格输入生成、oracle、误差界、比较和 main。
- [fast_math_kernel.cpp](../simd/fast_math_kernel.cpp)：fast 编译的实际内核包装。
- [fast-math-build.md](../simd/fast-math-build.md)：多源/object target 接线及禁 LTO 选项，作者未改公共 CMake。

主线程已按该说明接入 numeric_fast_check 专项目标，完整 Windows preset 开启，实际 MSVC 构建和 CTest 通过，完整编译/链接日志在本机 `exercises/build/full-windows/fast-build.log`。没有新增第三方依赖。内核用现有 Threads/可选 xsimd 能力；xsimd 13.2.0 作者副本 commit 为 1f8dd9c8e162968d9b4ff0251c56d431b8777f36。原生 C++26 std::simd 仍无可用本机实现，**未验证**；GCC/Clang fast flags 不能记为本批次已测。

教学、测量正文新增 [05-light-heavy-scaling.md](05-light-heavy-scaling.md)，并更新 12–14 相关入口、SIMD 精度/诊断正文、九题角色说明、L3/Cap3 完整操作命令。所有变更均在原授权范围内。

## 正确性验证结果

本机 MSVC 19.51.36256.0、C++23 preview、/O2，Windows 11 10.0.26220，AMD64 Family 25 Model 97 Stepping 2，32 个逻辑 CPU。

| 验证 | 结果 |
|---|---|
| 严格 + xsimd：numeric_test 与九题 Reference | 全部 PASS |
| ASan + xsimd：numeric_test 与九题 Reference | 十项全部 PASS，并最终经共享 run_process 30 秒外部超时执行 |
| 七题观察 main | 既定小检查 PASS，不算全 Part 评分 |
| L3/Cap3 实现 main，严格与 ASan | 实际学生路径默认 exit=1，正确记录为未完成，非 Reference PASS |
| 两个独立 benchmark.cpp，严格与 ASan | 编译及小/默认输入正确性检查 PASS |
| 并行算法=0、xsimd=0 回归 | 基线和当时 Reference 通过，可选路径跳过；最终 Starter/驱动角色由主线程全量构建复验 |
| 严格 checker + fast kernel，含 xsimd | PASS |
| 严格 checker + fast kernel + ASan | PASS，外部超时执行 |
| 错配编译模式 | strict checker 按 fast 编译、fast kernel 按 strict 编译均在 #error 守卫失败，符合预期 |
| RunnerSelfCheck | 0=PASS，77=SKIP，7=FAIL；执行/退出两个挂起均 TIMEOUT=FAIL |
| SIMD 独占 leaf | 配置/构建/7 项 scalar 驱动验证 PASS |
| 文档检查 | 相对链接与 diff whitespace 检查通过；正式 CMake 目标/注册名已对照共享实现核对 |

验证目录是 exercises/build/numeric-review-p2-*；verification.log 记录严格/fast 编译选项与执行结果，fast 目录还保存两个单元的汇编。编译期 /GL- 与最终 /LTCG:OFF 禁止跨单元优化，严格 TU 不包含生产内核 inline 定义，避免混合 COMDAT 语义。补偿 oracle 不依赖 MSVC long double 比 double 更宽。

受控挂起来自 Python Event.wait 及 atexit 退出回调，由现有公共进程工具终止，没有 sleep 顺序证明或新框架。清理错误与 timeout 单独判断，先于 Starter 的预期退出码判断。

## 开发六组样本：完整保留，不作最终性能结论

以下六组在各自采样时由 runner 验证 CSV/检查/五样本结构通过；PASS 只代表该协议执行成功，不能表示环境空闲或最后源码已被独立复验。采样没有与本作者自己的编译重叠，但主线程同阶段在集成重建，没有整机负载隔离证据。

这些样本还早于后续静态 lambda 分派、独立 benchmark.cpp 入口与脚本超时修订，**不能作为当前冻结版本的性能数据**。旧记录中的 exe 路径当时指向驱动；现在 main 已是学生程序，必须使用新的 _benchmark 目标重新正式采样。所有旧 run.json/samples.csv 保留，没有删值或覆盖样本。

| 负载/规模 | 原始记录 | 状态与用途 |
|---|---|---|
| light / 1024 | run.json（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/l3/light-1024/run.json`） | PASS，开发观察 |
| light / 65537 | run.json（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/l3/light-65537/run.json`） | PASS，开发观察 |
| light / 1048577 | run.json（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/l3/light-1048577/run.json`） | PASS，开发观察 |
| heavy / 1024 | run.json（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/l3/heavy-1024/run.json`） | PASS，开发观察 |
| heavy / 65537 | run.json（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/l3/heavy-65537/run.json`） | PASS，开发观察 |
| heavy / 1048577 | run.json（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/l3/heavy-1048577/run.json`） | PASS，开发观察 |

每组 seed=42、一次外部暖机、五次独立进程样本、timeout=30。外层按 light/heavy、1024/65537/1048577 顺序；内部策略顺序相同且可复现：

| 轮次 | 策略次序 |
|---|---|
| 暖机 | unseq → seq → par → par_unseq → plain |
| 正式 1 | unseq → par → plain → par_unseq → seq |
| 正式 2 | unseq → seq → par → plain → par_unseq |
| 正式 3 | seq → par → unseq → par_unseq → plain |
| 正式 4 | par_unseq → unseq → par → plain → seq |
| 正式 5 | par → seq → plain → unseq → par_unseq |

下表单位全部为 ms，包含所有正式编号样本，不删离群值：

| 负载 | N | 策略 | 五个样本 | 中位数 | 范围 |
|---|---:|---|---|---:|---|
| light | 1024 | plain | 0.0012, 0.0011, 0.0011, 0.0011, 0.0011 | 0.0011 | 0.0011–0.0012 |
| light | 1024 | seq | 0.0011, 0.0011, 0.0077, 0.0011, 0.0011 | 0.0011 | 0.0011–0.0077 |
| light | 1024 | par | 0.0259, 0.0252, 0.0275, 0.0272, 0.0285 | 0.0272 | 0.0252–0.0285 |
| light | 1024 | unseq | 0.0012, 0.0011, 0.0011, 0.0011, 0.0011 | 0.0011 | 0.0011–0.0012 |
| light | 1024 | par_unseq | 0.0260, 0.0253, 0.0272, 0.0255, 0.0267 | 0.0260 | 0.0253–0.0272 |
| light | 65537 | plain | 0.0594, 0.0602, 0.0601, 0.0614, 0.0605 | 0.0602 | 0.0594–0.0614 |
| light | 65537 | seq | 0.0610, 0.0596, 0.0604, 0.0657, 0.0620 | 0.0610 | 0.0596–0.0657 |
| light | 65537 | par | 0.0830, 0.0876, 0.0811, 0.0806, 0.0919 | 0.0830 | 0.0806–0.0919 |
| light | 65537 | unseq | 0.0599, 0.0599, 0.0601, 0.0608, 0.0607 | 0.0601 | 0.0599–0.0608 |
| light | 65537 | par_unseq | 0.0827, 0.0846, 0.1125, 0.0849, 0.0832 | 0.0846 | 0.0827–0.1125 |
| light | 1048577 | plain | 0.9627, 0.9769, 0.9750, 0.9755, 1.0225 | 0.9755 | 0.9627–1.0225 |
| light | 1048577 | seq | 0.9801, 0.9760, 0.9741, 0.9661, 0.9840 | 0.9760 | 0.9661–0.9840 |
| light | 1048577 | par | 0.3919, 0.4416, 0.3728, 0.3736, 0.5100 | 0.3919 | 0.3728–0.5100 |
| light | 1048577 | unseq | 1.0702, 1.0058, 0.9725, 0.9726, 0.9862 | 0.9862 | 0.9725–1.0702 |
| light | 1048577 | par_unseq | 0.3691, 0.4975, 0.4468, 0.3740, 0.4223 | 0.4223 | 0.3691–0.4975 |
| heavy | 1024 | plain | 0.0437, 0.0445, 0.0445, 0.0443, 0.0443 | 0.0443 | 0.0437–0.0445 |
| heavy | 1024 | seq | 0.0441, 0.0441, 0.0445, 0.0445, 0.0449 | 0.0445 | 0.0441–0.0449 |
| heavy | 1024 | par | 0.0896, 0.0680, 0.0680, 0.0838, 0.1254 | 0.0838 | 0.0680–0.1254 |
| heavy | 1024 | unseq | 0.0444, 0.0441, 0.0449, 0.0451, 0.0443 | 0.0444 | 0.0441–0.0451 |
| heavy | 1024 | par_unseq | 0.0729, 0.0684, 0.0918, 0.0867, 0.0675 | 0.0729 | 0.0675–0.0918 |
| heavy | 65537 | plain | 2.8377, 2.8436, 2.8556, 2.8838, 2.8595 | 2.8556 | 2.8377–2.8838 |
| heavy | 65537 | seq | 2.8701, 2.8259, 2.8512, 2.8317, 2.8274 | 2.8317 | 2.8259–2.8701 |
| heavy | 65537 | par | 0.8448, 0.8114, 0.8805, 0.8597, 0.8516 | 0.8516 | 0.8114–0.8805 |
| heavy | 65537 | unseq | 2.8083, 2.8359, 3.0287, 2.8417, 2.8752 | 2.8417 | 2.8083–3.0287 |
| heavy | 65537 | par_unseq | 0.7705, 0.8438, 0.8221, 0.8669, 0.8903 | 0.8438 | 0.7705–0.8903 |
| heavy | 1048577 | plain | 45.9074, 45.6927, 45.7524, 45.4553, 45.2656 | 45.6927 | 45.2656–45.9074 |
| heavy | 1048577 | seq | 46.0568, 45.5487, 45.4011, 45.8861, 45.9368 | 45.8861 | 45.4011–46.0568 |
| heavy | 1048577 | par | 5.7062, 5.3699, 5.5223, 5.3306, 5.5626 | 5.5223 | 5.3306–5.7062 |
| heavy | 1048577 | unseq | 45.7436, 45.7965, 45.6018, 45.9750, 45.7987 | 45.7965 | 45.6018–45.9750 |
| heavy | 1048577 | par_unseq | 5.3539, 5.6468, 5.3768, 5.4828, 5.4495 | 5.4495 | 5.3539–5.6468 |

在这份开发快照里，heavy 的 plain/par 中位数在小规模为 0.0443/0.0838，中规模为 2.8556/0.8516，大规模为 45.6927/5.5223。这个变化可用于讨论固定开销与计算量的摊销；不能据此断言最终版本有某个加速倍数或用了几个 worker。light 的中规模 par 也更慢，说明策略收益依赖负载。light=1024 的 seq 样本 0.0077 高于另外四个 0.0011，仍原样保留。未做 CPU 亲和、频率控制或硬件带宽计数，不能将机制假设写成已测原因。

开发 L3 驱动 SHA-256：2e564b0a533a3140dbf22db23413aef307cf8bf11e080e0d415ea055e43c9504；当时 Reference SHA-256：1709047a05d105f24a960fd357461d0f5f7025b5c668b8d7f0a5c20877181ede。run.json 另保存每组采样时全课程源码 hash；它不是最终代码 hash，也不等同于后来同路径的新二进制。

## Cap3 元数据及历史记录

空任务原始记录（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/cap3/empty/run.json`）包含以下五样本，主要用来核对已知零 worker 的元数据。极短时间不解读为性能排名：

| variant | size | threads | details | 五个样本 ms |
|---|---:|---:|---|---|
| reduce_threaded | 0 | 1 | workers=0, caller=1 | 0.0001, 0.0002, 0.0002, 0.0002, 0.0002 |
| threaded | 0 | 1 | workers=0, caller=1 | 0.0004, 0.0004, 0.0004, 0.0004, 0.0003 |

Cap3 开发全组（本机归档 `exercises/build/numeric-review-p2-strict-xsimd/measurements/cap3/full/run.json`）的十个 variant 都被当前公共 runner 接受，包括 par/reduce_par/sort_par 的 threads=0。旧“runner 拒绝 0”的接线问题已经关闭，不再列为阻塞；0 仅指后端未测，不能编码已知空任务。

首轮 numeric-author-direct-strict*/results-* 全部保留。早期快数学整文件测试不再作为可靠验证；早期性能组及本轮开发组均不作为当前正式性能结论。主线程将在作者源码稳定后，用自己的 full-windows 构建独立采正式组。完整新命令见 [L3 README](../../exercises/L3_par_vs_seq_bench/README.md)与 [Cap3 README](../../exercises/Capstone3_parallel_compute/README.md)，两者明确使用 _benchmark.exe。

## 交回同一非作者的复验点

请原 reviewer 按下列哈希检查六项 P2、Starter/Observation 分类及 Faraday 命令收尾。作者没有调用原 reviewer 的通道，不能自填“同一非作者复验通过”；本记录是可直接复验的交接材料。复验后若需修改，按实际影响重新构建/测试，再由主线程统一质量索引与正式测量。

原生 std::simd 缺实现的边界保留；fast 多源目标已集成，最终正式样本仍由主线程独立执行。公共 CMake、全量构建和正式测量不由本作者并发修改或运行；作者交接后的文档接线状态由主线程同步，代码文件未因此改变。

## 最终源码与关键文档 SHA-256

以下是本次交接版本；本记录自身不参与自引用哈希。全课程 runner hash 是各组采样时快照，不能与随后变更的整个工作树混同。

```text
exercises/include/concurrency_study/numeric_kernels.hpp 8AC8559CA93AD337C81086E932EEFCF2BC4A4C04B69A0E9EE361D003695F79F2
exercises/runtime_tests/numeric_test.cpp 5DD8416DC6E0112ADEA6D15CF5CC8DBE1A21BD226F0557E6BA643CE41960F18A
exercises/K1_simd_basics/solution.cpp C86F305EEC33B464E3D3AADA59887AE43BD89EFE419620DA97DA502C7EAB5C50
exercises/L2_parallel_reduce_scan/solution.cpp F726A37EF2AB97E296B32667B3A4CFF2CC2499E80B78898B2062383506739AC2
exercises/L3_par_vs_seq_bench/main.cpp AC451E5B7C91758C9CEF93F65B1BCFC5F0F5B6E2AFC29ECB70152E50E6C082FC
exercises/L3_par_vs_seq_bench/reference.hpp 91C2D0C363FB685D1F5AFADC129D14F83586EA95071CF8A6F1BFEE28FC426FFA
exercises/L3_par_vs_seq_bench/solution.cpp 5447A786D2F667930125B8C3D2048370988714FC387B93F6CD7BD8665C3A4E23
exercises/L3_par_vs_seq_bench/benchmark.cpp 608F4820DE658C6C4C0E549397AB122FF585C0292B73C8DF668651A744F2FCAB
exercises/L3_par_vs_seq_bench/README.md 7E32432AA60B9B7AB70A0724518A14A4C17B84CB6418986A924313ED6D1CAD8A
exercises/Capstone3_parallel_compute/main.cpp 6ED79F73B1EA8538FDC06142B86BC1FCF41BA4F58007BFF65C56E0F9DD52DA96
exercises/Capstone3_parallel_compute/reference.hpp E1D70B15C088EB1126D6AA5C100DC40ACC143516AC148F921273751FE09C2EC2
exercises/Capstone3_parallel_compute/solution.cpp 3CB59BAA7F6D4463ED7E6E361B1D741208F25C91AA5F9C0286134B4EC0B60235
exercises/Capstone3_parallel_compute/benchmark.cpp A22D55B456DE7B4E56A2C1E91C332DA2FC4712C82125B5D91742F490D8E0F58A
exercises/Capstone3_parallel_compute/README.md D3E765C1050F94A35A7782CA2E1BACBFD79127127D9F243CB6D4BAA594C5A9A1
topics/performance/verify-numeric.ps1 D09249AE2179D32D6F8767DEA4FFDD2C1F54E2BB8BC4BD86CB1163E5E744BAEB
topics/simd/fast_math_api.hpp 18A66FB60739845956D76B937390563C3EF4C95E32822F1E56F1A90B76D9E60B
topics/simd/fast_math_check.cpp 13C534577AD4EA14C0685CF5D9BF03D0EEE41817D0632B49E34D3038AD477218
topics/simd/fast_math_kernel.cpp A24FBED113D13BD514E17B83FC208CE65422794670F16C74A79D8403B402F018
topics/simd/fast-math-build.md 04FB2E47CF99E0B21E94A6E239DB45A77D979F538119FFF90F8054F15E6C8ED6
topics/performance/05-light-heavy-scaling.md E13FE881B926633C0EAA5AEA54CD4F24CE420DD2F91A32A4A0C14A08A16A6EC5
topics/simd/04-diagnostics-and-bandwidth.md 65AF01C11621C6879FA492F9926727FACC70F6E7AB6330F3D9E61EA9E68944F8
```
