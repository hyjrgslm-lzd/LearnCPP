# G2 正式构建成本测量（2026-09-08）

结果：72轮全部PASS，包括12轮预热和60个正式样本。每组5个正式样本，预热不参与统计；原始记录没有删除失败或无收益配置。独立数据审核见[最终技术报告](../final-technical-review.md)。

## 协议、环境和版本

- 三种配置baseline/PCH/LTO，共同使用五个TU与同一语义检查；四种场景clean/noop/implementation/public-header。每轮独立源码和构建目录，Release，Ninja并行度1，随机种子20260908，每条命令timeout180秒。
- clean计入configure和build的进程耗时之和，其他场景只计build；准备、复制、语义运行与driver写日志不计入。进程启动、执行和verbose输出收集计入，不能当纯编译器CPU时间。
- Windows11 x64，AMD Ryzen 9 9900X，12核/24逻辑处理器；CIM报告的物理内存总量203533176832字节。CMake4.2.3、MSVC工具集14.51.36231、VS Ninja1.13.2、Python3.13.11；实际编译/链接选项保留在每轮verbose输出中。
- 测量前本任务的其他构建与测试均已结束。常规系统服务仍运行，没有清缓存、设置亲和性/NUMA位置、改变电源策略或关闭后台服务。硬件信息于测量结束后从Win32_Processor/Win32_ComputerSystem读取；MaxClockSpeed不是采样中的实际频率。

输入fixture SHA256：`b27230ad278ac603d5b884cb25df35fd850d54db341a7b10c5cd5a8b9fe08fc8`；结束后重新计算一致。公共runner SHA256：`77f7aa8a86ff5fa4fdb933c8bb9929d637748a1aa00fb7afb3dac941e47d936b`。每轮源和exe另有各自指纹，副本中的无语义注释标记是预定输入变化。

## 原始结果（毫秒）

| 配置 | 场景 | 样本数 | 中位数 | 最小值 | 最大值 | 极差 |
|---|---|---:|---:|---:|---:|---:|
| baseline | clean | 5 | 5615.881 | 5454.178 | 5733.558 | 279.380 |
| baseline | noop | 5 | 116.413 | 100.167 | 120.506 | 20.340 |
| baseline | implementation | 5 | 970.767 | 846.444 | 1037.214 | 190.770 |
| baseline | public-header | 5 | 3567.950 | 3504.324 | 3720.926 | 216.602 |
| pch | clean | 5 | 4131.357 | 3852.660 | 4352.239 | 499.579 |
| pch | noop | 5 | 99.417 | 96.351 | 106.695 | 10.345 |
| pch | implementation | 5 | 486.008 | 482.810 | 844.853 | 362.044 |
| pch | public-header | 5 | 2204.199 | 2091.266 | 2297.299 | 206.034 |
| lto | clean | 5 | 5559.463 | 5440.456 | 5981.887 | 541.431 |
| lto | noop | 5 | 123.209 | 112.024 | 154.694 | 42.670 |
| lto | implementation | 5 | 1035.652 | 971.172 | 1073.969 | 102.797 |
| lto | public-header | 5 | 3657.246 | 3596.858 | 3991.271 | 394.413 |

clean阶段的分项中位数（分项中位数相加不保证等于总耗时中位数）：

| 配置 | configure | build |
|---|---:|---:|
| baseline | 1919.299 | 3709.299 |
| pch | 1920.621 | 2210.736 |
| lto | 1878.675 | 3683.381 |

## 解释与允许的结论

实际命令显示：所有no-op轮没有编译且Ninja报告no work；implementation轮只重编common.cpp并链接；public-header轮重编五个TU，PCH配置另有一次PCH生成。clean同样分别为5次/6次编译操作。三个配置每轮都通过同一语义运行检查，因此这里比较的是相同功能下的构建工作。

本机五TU负载中，PCH降低了三种需要编译场景的中位数，但仍有生成和失效成本；这些数据不证明所有项目都应开启PCH。LTO在本组构建耗时中没有一致优势；没有测量程序运行吞吐，不能据此否定或声称LTO的运行期收益。no-op没有解析或重编工作，其小幅差异不能归因于PCH解析收益。

configure约1.9秒，已经包含在clean总耗时；把clean与增量场景直接相除没有优化含义。verbose操作数证明依赖扇出与重编范围，分项进程计时展示构建阶段成本；它们不单独证明CPU微架构瓶颈、纯解析时间或统计显著性。五个样本只描述本次观测，保留范围以呈现抖动，不预设单调加速。

## 复现与证据

- [完整JSON：参数、随机顺序、每轮状态/命令/指纹及全部样本](g2-formal-r1/report.json)
- [实际顶层调用](g2-formal-r1/invocation.command.cmd)：本机历史入口包含固定工具路径和已存在output，复现时按当前机器选择路径并使用新目录；不要覆盖这批数据。
- [硬件与采样约束](g2-formal-r1/hardware-and-policy.json)
- [逐命令stdout/stderr/退出/timeout/cleanup记录](g2-formal-r1/raw/)
- [通用学习者命令与协议](../../../exercises/G2_build_cost/README.md)

大量源码副本、对象及exe保留在本机ignored build工作目录，不作为仓库交付文件；公开driver可重新构造，源码/二进制指纹和原始文本记录随课保留。

