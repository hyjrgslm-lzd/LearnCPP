# C07 质量与验证报告

约定的本机交付已完成：正文、实现、整课/叶级矩阵、独立 good 修复、正式测量与非作者复审闭合。标准库行为差异及实验边界保留，不宣称所有平台或全部标准行为通过。

## 输入与边界

执行依据：[实施规格](implementation-spec.md)、根全局计划和当前重构指南。用户批准完整建设，并明确复用 C08 建好的 WSL。初始与并行修改见 [工作树基线](validation/session-baseline-20260910.json)，其中 C08 修改不属于本课交付。

新增 19 章正文（00–18）和 11 个独立构建单元：L01–L09、P1、B01。十个实现型单元提供 Student、Reference、good、行为型 bad、检查器及逐 Part 解析；B01 是观察与实验单元，运行控制不代表完成分析。覆盖入口见 [coverage](coverage.md)。

资源文件处理贯穿同步读、映射读、真实完成式读和独占新文件写入；页保护、进程/IPC、字节锁、readiness、动态装载保留局部机制实验。L08 的可移植完成账本与原生驱动分开；P1 直接读取原始文件，不用 echo 或模型冒充 IOCP/io_uring 后端。

课程外只补根 README、全局计划与 C08/C09/C10 README 的导航/状态。保留既有及并行改动；不改通用指南、C01/C02 共享工具，不提交推送。二进制、CMake 缓存和第三方源码留在本机 build/guest，仓库证据保留文本、JSON、JUnit 和 hash。

## 环境与前置证据

| 检查 | 本次事实 | 证据 |
|---|---|---|
| WSL 环境 | 复用 LearnCPP-C08-Ubuntu-24.04，GCC13.3、CMake3.28.3、Python3.12.3、kernel6.6.87.2 | 初始命令回读；正式矩阵另存完整元数据 |
| Windows 运行器 | 成功、精确反例、错诊断、超时及自启子进程清理校准通过 | [校准](validation/runner-windows-initial.json) |
| Linux 运行器 | 相同四项控制在 WSL 实际通过 | 前置实际运行；正式矩阵另存运行器记录 |
| liburing2.15 | 独立源码构建成功，固定 commit 与版本头回读一致 | [隔离准备记录](validation/uring-setup.json) |
| Linux 基础 L01/L02 | fresh ext4 快照下 Reference/good/bad-rejected 共 6/6 PASS | [foundation-r2](validation/foundation-r2/ctest.json) |
| 工具非作者审查 | 交付汇总误吞 FAIL 的缺陷已修复，原复现经非作者复验关闭 | [工具审查](validation/tool-review-20260910.md) |

Windows x64 实际使用 MSVC 19.51.36256.0、VS18 2026、CMake 4.2.3、Python 3.10.11；完整平台与编译选项见[Windows 环境](validation/final-windows-release-r1/environment.json)。Linux 为 GCC 13.3、glibc 2.39；kernel、CPU、工具、ext4 与 io_uring 选项见[Linux 环境](validation/final-linux-release-r1/environment.json)和[源码快照](validation/final-linux-release-r1/snapshot-manifest.json)。

复用 H:/wsl 的既有发行版，本课仅占用 `/root/learncpp-c07` 独立目录。liburing 2.15 固定 commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`，不安装系统包。运行命令在各 JSON；复现入口见[构建指南](../exercises/BUILD_GUIDE.md)与两平台 matrix 脚本，RunId 必须全新。

第一次 liburing 构建编译已成功，但执行 pkg-config 时发现 guest 缺少原生命令，入口误匹配 Windows Perl PATH 并以 127 失败；随后采用 CMake 原生头/库前缀，重新隔离准备通过。该失败不是 io_uring kernel 能力缺失。第一次 `/tmp` 工作目录后续未找到，原因未确证，因此使用独立 `/root/learncpp-c07` 并确认 ext4，不将观察归咎于文件系统持久性。

受限子 agent 曾遇到 taskkill Access denied。实际 timeout 必须保留 FAIL，不能为了让校准绿灯而忽略 cleanup_error；父代理在正常已授权本机权限下重新运行严格校准并通过。

## 整课与隔离验证

| 组合 | 实际结果 | 原始证据 |
|---|---|---|
| Windows Release | 48/48，0 FAIL，0 SKIP | [CTest](validation/final-windows-release-r1/ctest.json) |
| Windows Debug | 48/48，0 FAIL，0 SKIP | [CTest](validation/final-windows-debug-r1/ctest.json) |
| Windows ASan / RelWithDebInfo | 48/48，0 FAIL，0 SKIP | [CTest](validation/final-windows-asan-r1/ctest.json) |
| Linux Release，io_uring ON | 48/48，0 FAIL，0 SKIP | [CTest](validation/final-linux-release-r1/ctest.json) |
| Linux Debug，io_uring ON | 48/48，0 FAIL，0 SKIP | [CTest](validation/final-linux-debug-r1/ctest.json) |
| Linux ASan+UBSan，io_uring ON | 48/48，0 FAIL，0 SKIP | [CTest](validation/final-linux-asan-r1/ctest.json) |
| Linux Release，io_uring OFF | 42/42；6 个原生完成路径为 NOT_ENABLED | [CTest](validation/final-linux-core-r1/ctest.json) |
| Windows Student-only Release | 10/10 受控未完成失败，include/link/source/codemodel 审计 PASS | [完整审计](validation/final-windows-student-r1/student-verification.json) |
| Linux Student-only Release | 10/10 受控未完成失败，include/link/source/codemodel 审计 PASS | [完整审计](validation/integration-student-r2/student-verification.json) |
| Windows 独立叶级 Release | 11/11 configure/build/CTest，累计 47 项通过 | [逐叶记录](validation/leaf-verification-20260910T225908/windows/summary.json) |
| Linux 已稳定 9 叶 Release | 9/9 configure/build/CTest，累计 40 项通过 | [逐叶记录](validation/leaf-verification-20260910T150538Z/linux/summary.json) |
| Linux L06/L09 新快照补验 | 两叶 configure/build/CTest，7/7；合计 11/11 叶、47 项通过 | [新两叶记录](validation/leaf-verification-20260910T151328Z/linux-l06-l09/summary.json) |
| Windows L06/L09 新构建补验 | 两叶 configure/build/CTest，7/7，确认修复后的独立 good | [新两叶记录](validation/leaf-verification-20260910T231413/windows-l06-l09/summary.json) |
| 两 good 的六组合增量 | Win/Linux Debug、Release、ASan（Linux 同含 UBSan），各 2/2 | [归档与源 hash](validation/good-independent-r2/evidence-summary.json)、[12 条原始测试](validation/good-independent-r2/test-records.json) |

版本说明：上述整课 r1 是基线矩阵，最终版本在此基础上仅替换 L06/L09 两个 good 头，并用六组合增量及新叶级闭合。原始整课记录不改写为“修复后重跑 48 项”。作者曾为增量构建直接同步两个 good 到三个旧 Linux 快照；[快照 delta](validation/good-independent-r2/snapshot-delta.json)保留旧 manifest/current hash，旧 manifest 不再声称代表该目录现状。最终两叶另建全新快照；B01 的 13 个依赖输入不含这两份 good，正式采样另外逐项校验指纹。

48 项包含观察程序和精确反例拒绝，不能称为 48 个实现作业完成。Student exit 1 只证明未完成入口安全失败。当前 guest 的 ring/read/cancel 最小探测与主体都实际通过；OFF 不据此取得能力 PASS。检测器覆盖实际运行的安全路径，未穷尽所有竞态或硬件。

`C07_runner_controls` 由 CTest 的 60 秒超时兜底，学习实验另有进程外监督与独立 records JSON。监督器还回收失败程序遗留的自有临时文件；超时或清理错误不能当作成功的取消。

## 独立审查与边界

[非作者归档](validation/independent-reviews-20260910.md)保存作者/审查者分离、样章推进门槛、行为伪实现和工具缺陷的闭环。L03/L04/L05 现由 checker 观察真实资源；L06/L09 使用 child witness 和模块调用计数；L07 检查未知输入与准确背压前缀。P1 注入物化失败，验证剩余 3 个目标完成退休，并检查后续调用正常。

最终教学反查确认主讲、Part、综合项目、源码和 C08/C09/C10 有实质承接。L06/L09 good 与 Reference 相同的问题已用独立 owner/进程编排和 module RAII 实现修复；非作者再次源码审查并实际运行两 good、两 bad-rejected，4/4，通过并关闭问题。L05 观察入口与 B01 正式采样的文稿边界也已修复、复读通过。

本机 MSVC STL 的外部初始缓冲重置未满足 LWG3120 / N4950：[Windows 当前观察](validation/source-observation-windows-r2.json)为 `observed_reset=false`、`second_allocation_bad_alloc=true`，Linux 同项为 true。观察程序 exit 0 仅表示取得事实且 fresh-resource 对照通过；**该 Windows 标准符合性检查为 FAIL**，不能从 CTest 绿灯改称 PASS。B01 每批新建标准 resource，不依赖此复用行为；未修改本机 STL。详见[源码阅读](../chapters/17-source-reading.md)。

- P1 限于调用期间稳定的本地普通文件，最多 64 MiB、65536 chunks、16 个在途请求，是有界物化管线。
- flush/readback 证明 API 完成及本次回读，不证明断电持久性，也不推广到远程文件系统。
- Linux 教学 decommit 用 `PROT_NONE` 撤销访问，不宣称等价于 Windows commit charge 释放或页面丢弃。
- 取消 CQE 与目标 CQE 分别收束；无法确认请求退休的不可恢复边界使用失败退出 70，禁止带着仍借用的 buffer 栈展开。此策略不是生产服务恢复方案。
- 动态装载 checker 保留模块观察者，证明调用身份和 owner 责任，不声称观测最终物理卸载。
- Windows/WSL 成本分开描述；未执行裸机 Linux、长期压力、硬件断电或全局清缓存。

## 正式成本实验

先取得两平台基线并写[阶段定位与对照假设](measurements/baseline-assessment.md)，再执行 compare。每个平台 5 个 baseline case、17 个 compare case；每 case 1 个预热进程加 5 个正式进程。四 phase 共 **264 条原始记录，44 预热、220 正式**，全部 VALID，0 FAIL/UNKNOWN/SKIP；每个平台 baseline/compare 的 executable/source 指纹一致。

| 平台 | 原始基线 | 原始对照 | 环境 |
|---|---|---|---|
| Windows | [30 条](measurements/b01-windows-baseline/result.json) | [102 条](measurements/b01-windows-compare/result.json) | [工具/二进制](measurements/environment-windows.json) |
| WSL/Linux | [30 条](measurements/b01-linux-baseline/result.json) | [102 条](measurements/b01-linux-compare/result.json) | [工具/二进制/liburing](measurements/environment-linux.json) |

主代理从每个原始 stdout 独立复算 median/min/max/样本标准差，检查五个正式样本、完整进程退出/临时目录清理及当前源码 hash，见[算术复核](measurements/arithmetic-audit.json)。详细阶段和计数解读见[成本分析](measurements/cost-analysis.md)。两平台按顺序独立采样，本任务编译已停止；未控制整台机器的后台负载、电源或亲和性。baseline/compare 分 phase，保留阶段漂移限制，不作统计显著性或普遍加速结论。

## 最终复核与冻结

[非作者最终证据复审](validation/final-evidence-review.md)独立回读矩阵、Student、增量、叶级和原始样本，重算统计与当前源/二进制 hash，并全量比较三个旧 Linux 快照：每个 131 文件，仅登记的两份 good 改变，0 缺失。教学与实现问题已关闭。

[交付 manifest r2](delivery-manifest-r2.json)记录本课正文、代码、构建入口和保存证据的逐文件字节 SHA256，排除 build、缓存、二进制及清单自身；清单中的工具校验仅证明链接、记录和指纹，教学深度由独立审查单独负责。[导航与范围核对](validation/navigation-audit.json)记录课程外 5 个导航文件及其完整文件指纹，这些文件保留了其他任务的既有修改。

初次清单之后检查到 6 份 `LastTest.log` 受仓库全局 ignore 规则影响。本课 `.gitignore` 精确放行验证日志，构建产物继续排除；[原 r1 清单](validation/delivery-manifest-r1.json)原样存档，当前核验以 r2 为准。

C07 作者文本无尾随空白；根 README 与 C08/C09/C10 README 的定向 `git diff --check` 通过。全局计划的 C07 新行另检无尾随空白，其他课程已有的尾随空白没有顺带修改。未暂存、提交或推送；本机 build 与 WSL 依赖不是仓库交付文件。
