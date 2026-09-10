# C08 本轮审计清单

本表以当前两份总纲为准；历史通过只对应历史范围。状态在取得本轮证据后更新。

| 编号 | 已定位事项 | 当前证据 | 处置/验收 |
|---|---|---|---|
| A01 | C++29线程属性、HP batches只有索引 | standards-and-implementations 的独立状态表；全局计划7.2 | 新专题、主体与独立probe，非作者审查 |
| A02 | C05异步日志交接悬空 | C05第20章及coverage；C08无spdlog专题 | 完整日志专题与独立学生验证 |
| A03 | 标准HP/RCU/sender只有probe | cmake/feature_probes.cpp；M2使用固定stdexec | 补原生主体，已有三设施复用 |
| A04 | 旧缓存脚本路径失效 | build/c01-concurrency-check 两个tools测试仍指向Concurrency_Study | 新目录重新配置构建与运行 |
| A05 | 学生good/bad证据主要在临时目录 | 同步/回收历史VALIDATION及当前文件树 | 16实现题可公开重放资产与include审计 |
| A06 | 演进定位证据与新版门槛需对齐 | 队列02以假设热点开篇；最终样本多为总耗时 | 样章成本定位及受影响正式复验 |
| A07 | 专题阶段记录与最终记录衔接不清 | 队列08/VALIDATION写待审；final-20260908存在5队列组 | 保留旧记录，补最终/本轮回链 |
| A08 | Linux/TSan缺整课当前结果 | 原质量报告平台边界 | 专用WSL2环境，能力控制及约定矩阵 |
| A09 | 默认Python3.10不满足runner要求 | digest_file实际AttributeError；已找到可运行uv Python3.13.11 | 显式选已安装解释器，提前版本诊断 |
| A10 | 版本与导航计数需本轮刷新 | 当前材料检查145文档/50题；旧报告144文档 | 最终机器输出、源hash、审查版本对应 |
| A11 | GCC拒绝队列默认lambda hook的无参推导 | linux-preliminary首批编译失败，临时最小转发重载推进编译 | 实际源改显式单参数转发，保留带hook模板；新Linux Release/Debug/ASan构建已消原错误 |
| A12 | Clang模块扫描工具缺失与原子运行库链接 | 首批Clang配置找不到scan-deps，关闭扫描后出现__atomic_is_lock_free未定义 | 本课无module接口，明确关闭扫描；Linux Clang链接atomic，下一批配置/构建已通过 |
| A13 | TSan专项仍有失败待归因 | scheduling全局new符号冲突；B3超时；F3 fence发布报告竞争 | 独立最小复现与正常/故障控制，区分课程缺陷和检测器边界，不添加掩盖原实验的同步 |
| A14 | Python参数被PowerShell拆分后工具检查静默缺席 | windows-full-r1/asan-r1配置记录的argv出现Python3_EXECUTABLE=C:与另一个路径参数，缓存同样截断 | 参数整体引用；根runtime配置要求Python3.10+，缺解释器失败；之前受影响结果只作C++子集证据，最终重新完整配置/构建/测试 |

样章与分片审查已逐步收敛：队列r2和日志r2经非作者复验批准，原失败记录保留；前沿clear语义及Reference OFF修订后仍需最终复验。学生资产和TSan专项在完成门槛内，不因其他配置通过而遗漏。

已读代表链：执行/对象与结果、同步/关闭与线程池、内存模型→发布→回收→动态队列、队列预分配/批量/SPSC、安全回收学生路径、SIMD/并行/调度的现有实验及六篇固定源码导读。P2/Q2属于观察题，不能把其运行成功说成独立学生实现完成。当前静态审计未发现可直接定位的公共队列/池/回收正确性阻断，不表示覆盖任意交错；后续真实验证新发现追加本表。
