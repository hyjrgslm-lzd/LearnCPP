# 质量与验证报告

> 2026-09-09目录与顶层项目名已迁移；本报告原验证结论及SHA绑定当时版本。新路径验证、项目命名与历史记录格式边界见[目录迁移记录](../../C02_Objects_Lifetime_Ownership/references/directory-migration.md)。

课程正文、代码及已知审查阻断已完成修复并通过非作者复验。以下记录实际完成的范围，不把作者交付、观察程序成功或能力跳过当作全部验证通过。最终公开数据与报告的集成复核状态见文末。

## 1. 交付范围与技术基准

本次包含18个递进入口、50个练习及其 Reference、复杂主题系列、七个独立测量驱动（五个公共驱动与两个项目驱动）、严格判分/fast 内核专项，以及六篇固定版本生产源码导读。原讲义保留迁移入口；知识去向、先修、契约与实现/导读深度见[覆盖表](coverage.md)。

主线从背景、基础进入原理与应用；复杂主题沿正确基线、问题、改动与验证展开，不把不同保证的分支混作一条性能升级链。自测和必做 Part 有解析，学生作业不通过修改 Reference 核对自己。

规范固定 C++26 N5050，C++29 另标。默认 C++23 是代码与 target 要求，不等于本机精确编译选项；版本与来源见[标准索引](standards-and-implementations.md)。

## 2. 实际环境

- 日期：2026-09-08；Windows x64 / 10.0.26220，SDK 10.0.26100。
- Visual Studio 2026 Community，MSVC 19.51.36256.0；CMake 4.2.3，Python 3.14.3。
- 本机 CMake 将 C++23 target 映射为 `/std:c++latest`；专题还分别用 `/std:c++23preview` 验证，不将预览选项当作完整库能力。
- AMD64 Family 25 Model 97 Stepping 2；32个逻辑 CPU、16组 SMT 核心关系、一个 NUMA node 0，基础页4096字节。未把硬件干扰尺寸常量当成实测一致性粒度。
- xsimd 13.2.0：`1f8dd9c8e162968d9b4ff0251c56d431b8777f36`。
- stdexec nvhpc-26.05：`6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。

独立 reviewer 还对部分临时完成体使用 Clang 22.1.3 检查；这不是 GCC/Clang/Linux 全课程验收声明。Windows 之外的完整构建及 NUMA 系统调用仍未作为本次整套实机门禁验证。

## 3. 最终构建与运行证据

检查在 Release 下有效，自动运行有进程外上限。主线程在修复后重新构建两种配置，原始输出随仓库保存：

| 检查 | 实际结果 | 证据及含义 |
|---|---|---|
| 离线核心 | 60项：58 PASS、2 SKIP、0 FAIL | [JUnit](measurements/ctest-verify-core.xml)；M2 未开可选依赖，N1 缺多节点条件 |
| 完整 Windows | 61项：60 PASS、1 SKIP、0 FAIL | [JUnit](measurements/ctest-full-windows.xml)；真实 xsimd/stdexec 与 fast 专项运行，N1 仍部分跳过 |
| 50个 main 入口 | 16个实现型预期返回1；34个观察型返回0 | [原始记录](measurements/entry-roles.json)；空作业未被记为完成，观察不代表全部 Part |
| 公共 Python 工具 | 10项检查通过 | CSV、独立重复次数、数值、状态、超时清理与导航反例；[test_tools.py](../exercises/tools/test_tools.py) |
| 原生 C++26 能力 | 开启探测后六项指定接口均不可用 | [完整原始诊断](measurements/native-probes.json)；与关闭选项区分 |
| 文档导航与登记 | 144份课程文档、50题、0错误 | [检查器](../exercises/tools/verify_materials.py)；不代替教学/技术审查 |

核心60项由50个 Reference、8个 C++ runtime、2个工具检查组成；完整配置再增加 numeric_fast_check。两配置数量不能加总成121个不同练习。

fast 专项实际命令确认：内核 `/fp:fast /GL-`，独立判分单元 `/fp:strict /GL-`，链接 `/LTCG:OFF /OPT:NOICF`。非作者额外强制开启全局、四种标准配置和自定义 Audit 配置 IPO，确认专项目标仍关闭 IPO，而普通目标保留原设置。见[构建指南](../exercises/BUILD_GUIDE.md)与[分离构建说明](../topics/simd/fast-math-build.md)。

原生探测覆盖 SIMD、senders、HP、RCU、atomic min/max、inplace stop 的指定接口；阴性结果不能外推为编译器完全不支持某领域。固定第三方通过也不等于原生标准接口通过。

## 4. 非作者审查与修复闭环

下表记录实际复验，不是作者自评。审查者未参与对应内容创作；共享构建由主线程实施、另由非作者检查。正文、代码、答案、工具、实验和迁移均纳入审查。

| 批次 | 非作者 | 已关闭发现及复验范围 |
|---|---|---|
| 队列样章 / Q0 | 独立样章审查者 | 容量1、消费者异常取消/join、最终为空检查；样章及契约通过 |
| 执行与结果 | Archimedes | tuple/apply 前置、D3关闭积压/FIFO/拒绝状态、D2超时观察边界；返修及编译检查通过 |
| 同步与关闭 | Lorentz | 十题从直接运行答案改成真实学生路径；空入口、八个完成体及错误操作反例独立验证 |
| 原子与内存模型 | Maxwell | atomic_ref 版本边界、原生选项关闭/探测失败区分；正文与协议通过 |
| 队列、栈与 ABA | Confucius | SPSC 压位 bool 存储改为独立对象；赋值约束匹配 const 源；Release/ASan回归和八版驱动检查通过 |
| 回收协议 | Arendt | worker启动异常、主线程异常展开与门闩/析构等待环；独立重建 Release/ASan 执行故障场景 |
| 回收学生路径 | Arendt | 22文件复验；空入口、完成体、17单TODO、4空清场及6并发异常变体；54次负向检查覆盖Release/ASan，脚本模式亦独立验证 |
| 性能与 SIMD | Raman | 哨兵/skip-write、严格oracle与fast隔离、空任务线程字段、轻重负载、外部超时、目录、选做SIMD；严格/ASan、模式反例和完成体通过 |
| NUMA 与调度 | Poincare | 独立reservation与统一布局、异常寿命、Linux相位模型、专用绑核线程、N1逐Part汇总；模型、故障/ASan、CPU/页检查及13代码指纹通过 |
| 公共工具与构建 | Descartes | CSV/重复进程/部分跳过/超时清理、题型、独立benchmark、fast多源与配置级IPO；配置及反例检查通过 |
| 教学试做 | Faraday | 六条代表路线两遍推演；L3编号/命令、I2作业与脚本映射修复后通过；15段PowerShell语法及21文件指纹匹配 |
| 导航、迁移与源码 | Darwin | 六篇真实源码路径；moodycamel可抛/nothrow发布顺序、mimalloc失败重入链；固定源码与覆盖复验通过 |

具体版本证据：

- 同步：44个产出及记录核对；[同步记录](../topics/synchronization/VALIDATION.md)批准时 SHA-256 为 `4BD409DA8FEE839B88C88CE61A4DF522C91A671EB0E5359D06F141EA20E2726F`。
- 回收：[21文件清单](../topics/reclamation/student-paths.sha256)全部匹配；清单自身 `58062c88ef77cc7035d72dac9543550749f830a7228f1b823d8d810cba2cbea2`。13个原协议文件未因学生路径修订改变。
- 数值：[21项清单与复验点](../topics/performance/VALIDATION.md)匹配；最后批准的 Cap3 main 为 `6ED79F73B1EA8538FDC06142B86BC1FCF41BA4F58007BFF65C56E0F9DD52DA96`。
- NUMA：[13代码指纹](../topics/scheduling/verification.md)匹配，清单摘要 `7136B85DEDDC4F513A57BE980247D986797C8A69C9AB1E6F25DD5516815769E6`。独立 ASan 首次启动超时，补齐该子进程已安装MSVC运行库搜索路径后通过，不隐去首轮超时。
- 源码：02修复后为 `CDC1BE0F2414216FB64DE1CB5A2630674CC47F1380DE917716C429D7252B89DE`，04为 `459CCDD77402CA70C917341430B26022214252859FA8DC29EC3C4E6A38E9EE1C`；固定commit、文件和符号在正文。没有运行这些生产库。

专题记录中的“作者交接时待审”具有阶段含义；本表补充后续独立结论。原始会话不随源码发布。指纹绑定具体文件快照，换行差异也影响字节值，不能无条件等同于任意平台检出后的字节。

教学试做覆盖 P2、C2、G3、I2、L3、N1 关键 Part 和状态推演，不冒充盲测全部50题。P2/G3 的部分解析夹在任务中，第一遍只称“不看实现的试做”，不称完全遮住所有答案。

## 5. 最终正式采样

[实验附录与原始数据](measurements/README.md)保存17组：365次正式进程中345 PASS、20 SKIP、0 FAIL；另有73次预热及36次前置正确性运行。全部样本保留，15组 PASS、2组 PARTIAL_SKIP；部分跳过组原生退出码为77。

按线程角色、容量、工作量、顺序及失败契约分组；SIMD 的加法/点积/布局、项目的 GEMM/归约/排序分别解释。保留预分配或缓存无稳定优势、xsimd点积更慢、小矩阵线程化更慢等结果，不删掉不支持优化假设的数据。

采样在停止本任务其他构建和运行后由主线程串行执行，未控制操作系统所有后台负载或频率。17组使用同一runner源码快照；后续新增报告会改变包含文档的全树hash，原始样本不会为迎合新报告而回写摘要。

Descartes 已独立重算全部统计、核对 CSV/JSON、随机顺序、23个不同可执行文件的当前哈希、验证输出及附录结论，批准39个公开证据文件；集合摘要为 `f7ae29985a8075b4da84a7b3499006e0230777118dcb4d98cddbd7e652e27086`。本报告另由 Darwin 独立核对范围、计数、专题指纹及证据边界后通过。

## 6. 未验证与明确边界

- 无多节点硬件，remote、interleaved、跨节点共享读取未取得实机证据。N1记录3个Part PASS、3个Part SKIP，整体77；单节点和纯模型不替代远端验证。
- Linux NUMA 系统调用、Windows多processor group全范围、连续迁移/fault轨迹、真实OS线程耗尽、线程池构造内部实际OS失败尚未实测。
- 原生六项接口未过探测；未强行开宏造通过。TS、固定第三方和标准路径分别标注。
- ASan不是数据竞争证明，本次无完整TSan平台运行。有限历史、压力测试与协议推导分别看待。
- 源码导读限定固定快照选定路径，不证明所有模板组合、生产稳定性或全部上游版本。
- SIMD输入域、误差预算、ISA、对齐/尾部/索引边界仍是调用前提；未测实际带宽及完整硬件计数器。
- 已知审查阻断关闭，不等于穷尽任意执行或平台。

## 7. 集成与发布状态

代码、正文、最终公开数据及本报告均已完成非作者独立复验，已知阻断关闭。通用重构指引按本次要求在课程完成后另行编写和审查；提交、推送核验及正常关机按授权顺序执行，实际发布与系统操作由最终交付消息确认，不在本报告中预先宣称。

保留用户原有 Coroutine 学习代码。专题里的本机构建归档不冒充随仓库发布；可随源码读取的证据在 measurements 目录。

## 8. C01 衔接补充（2026-09-08）

本批仅补 README/覆盖表的 C01 先修、C08 主讲/C13 实验资产归属，以及标准索引的 C++29 线程属性与 HP batches。50题、算法、原测量没有改动；上述历史批准仍只对应原来列明的范围，新索引不代表两项前沿接口已经实现。

主负责人从新目录 `build/c01-concurrency-check` 配置、构建并运行核心验证，显式使用已安装 Python 3.13.11，实际编译器 MSVC 19.51.36256.0。结果为60项中的 **58 PASS、2 SKIP、0 FAIL**，包括材料检查和工具自检。M2未请求固定stdexec，N1仍缺多节点条件；没有重采未改动的benchmark。

本轮命令、退出码和输出见 [configure](validation/c01-supplement/configure.json)、[build](validation/c01-supplement/build.json)、[CTest](validation/c01-supplement/ctest.json)与[JUnit](validation/c01-supplement/ctest.xml)。构建输出由既有runner按UTF-8收集，部分本地化MSBuild诊断含替换字符；返回码、英文目标与测试结果完整，这份文本不用于还原原始中文诊断字节。

本批新增文字及证据已由非作者独立核对并 APPROVE，见[本批独立审查](validation/c01-supplement-independent-review.md)：复核了官方入稿依据、60项计数/两项SKIP、工具检查和文件指纹。该审查没有重新批准未改动的全部历史算法或外部平台；全仓新增课程的最终集成审查另行进行。本段是取得审查后的状态回填，与报告中记录的被审快照区分。
