# C08 本轮执行交接

本轮内容、代码、Windows/WSL约定矩阵、学生验证与正式采样均已落盘。最终是否签收以[本轮报告](../../revision-quality-report-20260910.md)所链接的非作者集成审查为准；本文件不复制容易过期的计数。

- 原八个dirty文件保护见initial-snapshot.json；C08原395跟踪文件指纹保留。
- 主要C++/CMake输入冻结见final-code-snapshot-r2.json；首次快照及M1单项增量均保留。
- Student r1/r2脚本和Windows/Linux原始过程分别保留，r2包含Ninja Release与POSIX依赖解析修复；不覆盖失败记录。
- 正式队列数据见references/measurements/c08-revision-final；计数诊断和计时分离，5组40正式样本/8预热/12正确性进程。
- 原生标准库、多NUMA节点与四项已验证TSan边界分别登记；模型和部分执行不冒充完整PASS。
- 旧缓存、Python参数拆分、临时快照缺头、模板/链接问题、检查器及语义返修记录都保留；最终采用明确标出的有效版本。

本轮只修改获批C08范围、C05日志回链、根导航与全局计划C08状态；不提交、不推送、不关机。WSL专用镜像保留供复验，没有迁移Windows主仓库。后续先读取本轮质量报告和交付清单，再选择实际需要重验的能力，不从旧临时目录重新开始。
