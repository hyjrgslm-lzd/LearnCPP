# B01正式实验结果

正式数据：[完整报告](b01-formal-20260910-143713/report.json)，[原始样本](b01-formal-20260910-143713/raw/)。本次336个独立进程：56次预热、280次采样；56组每组5个有效样本，全部通过独立oracle，源码/程序/驱动指纹无漂移。

## 环境与口径

Windows x64 build 26100，24个逻辑CPU；MSVC19.51.36256.0、STL145/202604、CMake4.2.3，Release C++26。实际可执行文件来自整课build-vs2026/B01_cost/Release。固定seed42，N=256/8192，有效率或命中率目标10%/90%；随机数据的实际数量见每轮payload。

计时与诊断使用同一算法的不同插桩实例。下表只比较timed；counted只用于操作归因。初始化和计时边界遵从B01题面；build与lookup分开列，不把查询快误称为总成本一定低。开始快照未见编译进程，本任务测量期间未构建；没有全程控制整机所有后台负载，结果只适用于记录条件。

## 日志查询与收束（毫秒）

| 场景 | 实现 | 中位数 | 最小—最大 |
|---|---|---:|---:|
| large_high_valid | loop | 0.6218 | 0.5416—0.7603 |
| large_high_valid | materialized | 1.2890 | 1.1641—1.7059 |
| large_high_valid | reparse | 1.4283 | 1.2699—1.8611 |
| large_low_valid | loop | 0.1511 | 0.1400—0.1669 |
| large_low_valid | materialized | 0.3406 | 0.3085—0.3711 |
| large_low_valid | reparse | 0.3055 | 0.2818—0.3786 |
| small_high_valid | loop | 0.0241 | 0.0222—0.0311 |
| small_high_valid | materialized | 0.0514 | 0.0481—0.0667 |
| small_high_valid | reparse | 0.0496 | 0.0462—0.0535 |
| small_low_valid | loop | 0.0085 | 0.0059—0.0117 |
| small_low_valid | materialized | 0.0240 | 0.0164—0.0250 |
| small_low_valid | reparse | 0.0117 | 0.0085—0.0133 |

四组输入中，显式循环基线都胜过这两种Ranges组织方式；实体化并不总优于复算。例如large_high_valid为0.6218/1.2890/1.4283 ms（loop/materialized/reparse），large_low_valid的实体化反而比复算慢。该结果保留，不能把课程写成管道必然更快。

诊断组rep0、N8192、高有效率实际有7360条有效记录：loop与materialized各解析8192次；materialized额外保存8192个optional槽位；reparse解析23743次。这不是样章手动单遍消费的N+有效数公式：B01使用ranges::to，本版MSVC的__msvc_ranges_to.hpp:1115选择from_range构造，vector:730-733对forward范围先distance再构造，引入额外遍历。源码版本见[固定源码路线](../../source-reading.md)。计数与路径支持复算/实体化解释，不证明具体分配次数或cache miss。

## 索引（毫秒，构建和查找分开）

| 场景 | 实现 | build中位数（范围） | lookup中位数（范围） |
|---|---|---:|---:|
| large_high_hit | linear_vector | 0.0203 (0.0144—0.0265) | 14.3654 (14.1336—15.0386) |
| large_high_hit | map | 0.4109 (0.3943—0.4189) | 0.4362 (0.4273—0.4384) |
| large_high_hit | sorted_vector | 0.0574 (0.0524—0.0616) | 0.3759 (0.3711—0.3793) |
| large_high_hit | unordered_map | 0.2634 (0.2550—0.2755) | 0.0432 (0.0426—0.0451) |
| large_low_hit | linear_vector | 0.0193 (0.0154—0.0219) | 27.1354 (25.2466—29.5340) |
| large_low_hit | map | 0.3969 (0.3822—0.4656) | 0.4370 (0.4329—0.4657) |
| large_low_hit | sorted_vector | 0.0577 (0.0548—0.0701) | 0.3793 (0.3749—0.3899) |
| large_low_hit | unordered_map | 0.2613 (0.2511—0.2745) | 0.0341 (0.0338—0.0350) |
| small_high_hit | linear_vector | 0.0031 (0.0016—0.0042) | 0.0168 (0.0158—0.0259) |
| small_high_hit | map | 0.0210 (0.0188—0.0252) | 0.0075 (0.0073—0.0084) |
| small_high_hit | sorted_vector | 0.0054 (0.0045—0.0076) | 0.0078 (0.0074—0.0085) |
| small_high_hit | unordered_map | 0.0125 (0.0114—0.0136) | 0.0010 (0.0009—0.0011) |
| small_low_hit | linear_vector | 0.0035 (0.0015—0.0041) | 0.0263 (0.0261—0.0269) |
| small_low_hit | map | 0.0198 (0.0197—0.0217) | 0.0075 (0.0075—0.0078) |
| small_low_hit | sorted_vector | 0.0055 (0.0047—0.0072) | 0.0077 (0.0075—0.0078) |
| small_low_hit | unordered_map | 0.0129 (0.0116—0.0163) | 0.0008 (0.0006—0.0009) |

large_high_hit中，linear_vector构建约0.0203 ms、查询约14.3654 ms；unordered_map分别约0.2634/0.0432 ms。额外索引构建成本换取该批查询的较低成本，不能省略构建阶段或推广到其他键分布、更新率和查询量。sorted_vector是独立候选，不能因其查找不是最快就忽略其构建成本与有序能力。

counted与timed的耗时有时差异明显，本报告不把两者混排，也不从这种差异猜测编译器、分支或缓存根因。所有原始时间、负结果和离散范围均保留；没有设置最低加速比。

## 完整性

进程退出、超时、清理、JSON、全字段checksum/计数oracle均通过；报告内含采样顺序与运行前后五项SHA。后续G3校验修复不触及B01/CAPSTONE1或此程序，原始正式报告保持不变。前沿cache_latest等在本机未运行，未拿自写替代品作为标准库性能数据。
