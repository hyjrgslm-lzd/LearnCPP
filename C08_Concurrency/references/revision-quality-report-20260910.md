# C08 审计补全与本机验证报告（2026-09-10）

本报告记录已交付正文、代码和约定本机验证；最终独立签收结论见文末集成审查。所有通过只对应下面明确的环境与范围，不能把模型、观察、学生完成体、Reference或能力跳过混作同一种结果。

## 1. 本轮交付与知识去向

保留原十八个递进入口、五十题及队列/回收/内存模型/SIMD/NUMA/调度/源码导读。新增U01异步日志、F01线程属性、F02 HP batches，合计**53个普通练习入口**：原16实现型加U01为17实现型，原34观察型加F01/F02为36观察型。F03另外组织三个标准原生主体，不设空摘要可执行文件，不计作额外学生作业。

- 异步日志承接C05同步前端与C08队列/关闭；真实固定spdlog/fmt，覆盖三溢出策略、消息/线程归属、异常处理、flush和排空；Student/good/Reference/行为bad分开。
- C++29线程属性与HP batches补齐正文、模型、标准源码、解析和独立能力门。标准HP/RCU/sender不再只有probe；已有inplace stop/min-max/SIMD分支保留。
- 队列样章先补真实操作计数，再正式采样。诊断宏只给独立driver，正常算法和计时目标不计数；GCC默认lambda hook兼容问题用明确转发重载修复。
- 十六个原实现题具有公开可重放initial/good/bad，实际编译依赖审计；L3/Cap3只把输入/独立oracle抽到checks.hpp，保留学生函数体和TODO，Reference/benchmark仍独立。
- Windows/WSL各用本地文件系统工作区，补Sanitizer预设和工具登记门槛；TSan问题有课程外最小复现和明确未测项。

覆盖与先修见[覆盖表](coverage.md)，范围见[获批规格](revision-plan-20260910.md)，问题闭环见[审计清单](revision-audit-20260910.md)。C09帧协议、C10完整sender设计、C11服务观测、C13完整性能课程保留各自主讲责任。

## 2. 当前环境与输入版本

Windows11，AMD Ryzen9 9900X，12核/24逻辑CPU；MSVC19.51.36256、STL145/更新202604、CMake4.2.3、Python3.13.11。CMake的C++23目标在本机映射/std:c++latest；实际编译/链接选项保留在生成与构建记录。[Windows环境](validation/c08-revision/windows-environment.json)

WSL2.6.1专用Ubuntu24.04.4，kernel6.6.87.2，GCC13.3、Clang18.1.3、CMake3.28.3、Python3.12.3。源码/构建/依赖在guest ext4，VHDX位于H盘；[WSL指南](wsl-validation.md)记录校准、快照及使用方法。WSL结果不替代裸机性能或真实多NUMA节点。

固定依赖沿用既有提交：xsimd13.2.0、stdexec nvhpc-26.05，新增课程接线复用fmt12.1.0 `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`与spdlog1.17.0 `79524ddd08a4ec981b7fea76afd08ee05f83755d`，静态库/external fmt；核心默认关闭所有扩展且不下载。

[初始保护快照](validation/c08-revision/initial-snapshot.json)记录八个既有修改及395个原C08跟踪文件。最终主要C++/CMake输入见[243项代码快照r2](validation/c08-revision/final-code-snapshot-r2.json)，相对首版只新增M1已验证检测器边界；学生脚本另绑定r1/r2版本。完整交付文件与说明见[交付清单](revision-delivery-manifest.md)。

## 3. 最终验证矩阵

表中PASS、FAIL、SKIP按JUnit/实际进程分别核对，SKIP不加到PASS。跨配置数量不能相加充当不同练习数量。

| 验证 | PASS | FAIL | SKIP | 原始证据 |
|---|---:|---:|---:|---|
| Windows默认离线核心Release | 59 | 0 | 7 | [66项JUnit](validation/c08-revision/final-core-ctest.xml) |
| Windows完整依赖Release | 66 | 0 | 6 | [72项JUnit](validation/c08-revision/final-win-release-r2-ctest.xml) |
| Windows完整依赖Debug | 66 | 0 | 6 | [72项JUnit](validation/c08-revision/final-win-debug-r2-ctest.xml) |
| Windows ASan＋日志，RelWithDebInfo | 63 | 0 | 7 | [70项JUnit](validation/c08-revision/final-win-asan-ctest.xml) |
| WSL GCC Release＋日志 | 63 | 0 | 7 | [JUnit](validation/c08-revision/linux-final/m1-r2-final-20260910-181517-b97f497d/gcc-release-junit.xml) |
| WSL GCC Debug＋日志 | 63 | 0 | 7 | [JUnit](validation/c08-revision/linux-final/m1-r2-final-20260910-181517-b97f497d/gcc-debug-junit.xml) |
| WSL Clang ASan/UBSan＋日志 | 63 | 0 | 7 | [JUnit](validation/c08-revision/linux-final/m1-r2-final-20260910-181517-b97f497d/clang-asan-ubsan-junit.xml) |
| WSL Clang TSan＋日志 | 59 | 0 | 11 | [JUnit](validation/c08-revision/linux-final/m1-r2-final-20260910-181517-b97f497d/clang-tsan-junit.xml) |
| Windows前沿模型/原生主体 | 2 | 0 | 5 | [7项JUnit](validation/c08-revision/final-native-win-ctest.xml) |
| Linux前沿模型，独立直接运行 | 2 | 0 | 0 | [运行记录](validation/c08-revision/final-linux-frontier-models.json) |

Linux另五个原生主体均SKIP，见[原生JUnit](validation/c08-revision/linux-final/final-20260910-173010-d5356576/frontier-clang18-junit.xml)。两环境均明确请求八项C++26/C++29能力，全部未满足当前指定接口/flags的编译链接门槛；[Windows八项原始诊断](validation/c08-revision/capabilities-win/index.json)与Linux配置日志分别保存。OFF是DISABLED，ON后缺能力才是探测受限；F01/F02模型通过不补成原生PASS。

核心及完整矩阵实际注册并执行了`runtime_benchmark_tools`、`runtime_materials`。工具使用现有unittest；Linux的Windows专用进程回收控制按平台跳过，不冒充已验证Windows机制。Reference OFF及单题构建见[前沿复验](validation/c08-revision/reviews/frontier-review-r5.md)与下面学生记录。

### 学生验证

- Windows r1：16题×initial/good/bad=48个预期结果，分别16次安全拒绝、16次完成体通过、16次行为bad被同一checker拒绝。[原始summary和448个公开日志索引](validation/c08-revision/students-win-r1/README.md)
- 工具r2只修单配置Release参数和POSIX依赖解析，overlay算法不变；Windows I2/Cap3六项复验通过。[r2证据](validation/c08-revision/students-win-r2/README.md)
- Linux/Ninja r2：全部48个预期结果成立，16个good依赖追踪非空且无Reference/solution泄漏，编译命令含-O3/-DNDEBUG。[summary](validation/c08-revision/linux-final/students-r2-full-20260910-182216-452abc20/script-summary.json)、[公开逐case索引](validation/c08-revision/linux-final/students-r2-full-20260910-182216-452abc20/public-index.json)
- 新U01的Reference/good/两个行为bad在Windows Release/ASan、Linux Release/ASan/TSan均4/4通过；初态安全拒绝另验。bad不是修改checker期望，也不把崩溃、Sanitizer或超时当成功。

这些结果不表示学生已写完题目。Cap3公开good完成六个必做Part；选做SIMD明确未实现，主动请求返回1而非SKIP或scalar冒充SIMD。Reference真实SIMD保留并运行。

## 4. 问题闭环与证据边界

| 发现 | 最小证据与处理 | 独立复验 |
|---|---|---|
| 新日志原稿检查未消费Student、bad改了期望、异常清理/超时不全 | 改真实Submission接口、独立good和行为bad；先放闸再join，统一内部/外部超时 | [日志r2](validation/c08-revision/reviews/logging-review-r2.md) |
| HP batch clear语义反向、F03漏REF OFF及摘要假测试 | 按固定标准修成owned HP释放后empty；F03仅三独立主体 | [前沿r5](validation/c08-revision/reviews/frontier-review-r5.md) |
| 队列只有公开调用计数，不能定位预分配/缓存 | 默认OFF插桩实际lock/load，隔离operator-new热区；保留扰动说明 | [队列r2](validation/c08-revision/reviews/queue-sample-review-r2.md) |
| GCC hook推导、Clang扫描/原子运行库 | 原始失败先定位，明确单参转发、无module则关闭扫描、链接atomic | 新Linux配置/构建及[基础设施审查](validation/c08-revision/reviews/infrastructure-review.md) |
| 旧缓存路径及Python命令拆分导致工具缺席 | 新目录构建；参数整体引用；Python REQUIRED，缺解释器配置失败 | [缺Python反例](validation/c08-revision/missing-python-rejected.json)及最终两个tools实际运行 |
| 学生bad/覆盖旧输出/依赖解析问题 | 完整good基础上变行为、严格诊断、唯一run目录、真实依赖追踪；Linux问题另有前置失败 | [学生验收](validation/c08-revision/reviews/student-assets-acceptance.md)、[平台增量](validation/c08-revision/reviews/student-platform-review.md) |
| TSan once异常、fence、global new冲突 | 纯控制确认工具边界；保留可测部分，分离分配注入，不修改原协议加同步 | [TSan修复](validation/c08-revision/reviews/tsan-fix-review.md) |
| M1 what读取/worker清理报告 | 第一std probe留了main owner，不能复现；移除owner并控制worker最后清理后纯标准例11/11同形报告，值/ASan/plain对照通过 | [M1复验](validation/c08-revision/reviews/tsan-m1-review.md) |

仅已验证的Clang18＋libstdc++13头文件＋TSan组合跳过四个受限部分：B3异常重试、F3 fence发布、M1异常消息文本、全局new/delete注入。B3/F3/M1仍运行支持部分但整体返回77；调度并发runtime继续PASS。其他编译器/模式默认运行完整路径。没有suppression、额外join-before-get或全局安全参数变更来制造通过。

原生能力缺失、N1多NUMA缺条件、M2在离线配置未开固定stdexec是另外的边界。有限压力/历史搜索及工具不报警不证明任意交错安全；未测真实多节点、其他未声明工具链和完整生产库组合。LSP/ast-grep不可用已明示，以实际编译、CTest、受控反例和rg替代，不声称LSP clean。

初期失败不删除：baseline材料因并行U01未落完失败；asan-r1抽检查头后缺直接iostream；早期Windows配置Python路径拆分导致少工具测试；Linux第一批编译失败；Student只复制exercises漏I3专题支持头的36/48记录均保留。它们不作为最终通过证据。

## 5. 正式实验与审查

[本轮五组队列正式数据](measurements/c08-revision-final/README.md)使用最终Release、无诊断宏：8variant，40次正式＋8次预热＋12次前置正确性进程，全部PASS。每variant五次独立进程，seed42，全部样本和负面结果保留。ring/mutex范围重叠；缓存版SPSC本轮中位数反而更高；不同角色/失败/批量/分配契约不混排。源码级计数不当成硬件事件归因，也不与不同机器的历史样本计算加速比。

旧17组final-20260908封存不覆盖。非队列既有性能文字的归因边界经[独立审查](validation/c08-revision/reviews/performance-claims-review.md)核对；本轮正式统计/数据链另见[测量审查](validation/c08-revision/reviews/measurement-review.md)。

规格、样章、学生、技术、实验、教学导航分别由未参与相应内容创作的审查者复验；[教学集成](validation/c08-revision/reviews/teaching-integration-review.md)只批准其实际读取的教学结构，不代替最终数据。最终全部交付的一致性签收见[最终集成审查](validation/c08-revision/reviews/final-integration-review.md)。

## 6. 交付与后续

根README、全局计划只回填C08切片；C05仅增加异步日志回链，其他已有dirty文件及输入指南保留。构建产物、依赖checkout和WSL镜像只在本机；随仓库的是正文/源码/检查器/原始文本证据，raw记录中的本机路径不冒充远端可访问文件。

本轮没有Git提交、推送或关机。后续在具备新标准库/多节点硬件或更新检测器时，按原实验规格重新探测与验证，不直接继承当前SKIP或历史批准。
