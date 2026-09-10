# C07 非作者审查闭环

本文由主代理归档原生子任务的实际返回。代码作者没有批准自己的代码；各次局部运行是审查当时的证据，最终版本统一以质量报告的完整矩阵和冻结 manifest 为准。

| 审查范围 | 非作者 | 发现、修复与复验 | 结论 |
|---|---|---|---|
| L01/L02/L08 先修与复杂样章 | `c07_sample_review` / code-reviewer | Windows 严格 timeout 清理在正常本机权限下连续 5 次通过；Windows/Linux 样章各 13/13，遮答案独立 L02 candidate 1/1；取消后收束再释放上下文 | APPROVE 后才展开其余链 |
| L03/L04/L05 | `c07_sample_review` / code-reviewer | 初版报告布尔值可伪造；改为真实 region、映射 window、resource，由 checker 查询状态/构造对象/观察 upstream。三个兼容接口的伪实现重新被行为拒绝；Windows Debug/Release 与 Linux 各 9/9 | APPROVE |
| L06/L07/L09 | `c07_architecture_fallback` / code-reviewer | 修复不启动子进程、直接计算模块结果、预录 readiness 报告能通过的问题；使用运行时 challenge/PID witness、模块调用计数、未知 payload 与准确背压前缀。L06/L09 bad 各 exit 1 且命中专属诊断，四个正向程序 exit 0，复跑 6/6 | APPROVE |
| 验证工具 | `c07_architecture_fallback` / code-reviewer | 汇总吞掉 FAIL 的复现修复；Student 从 CTest 取完整 argv，包含模块参数；ext4 要求、临时目录清理和缺失诊断均保留真实失败 | APPROVE，详见[工具记录](tool-review-20260910.md) |
| P1 原生管线与 B01 协议 | `c07_pipeline_review` / code-reviewer | 原始文件真实 IOCP/io_uring；失败时取消并排空剩余请求。P1 fault control 验证 3 个剩余目标完成退休。采样器原来接受带 stderr 的成功 JSON，修复后独立复现为 UNKNOWN、不进入统计；self-check 通过 | APPROVE，可冻结采样 |
| 线程/进程观察、文件锁、Windows 矩阵脚本 | `c07_architecture_fallback` / code-reviewer | 线程同时存活且创建失败时释放 latch；Linux 使用真正 fork child 竞争进程关联锁；Windows 两 handle；positioned I/O 不推进 offset；Windows Debug 两项观察 2/2，Linux 纳入最终矩阵 | APPROVE |

P1/B01 最后一个 LOW 是旧 Windows 源码观察记录缺少新增字段。主代理用最终 Release 二进制生成[新 r2 记录](source-observation-windows-r2.json)，保留旧记录，更新源码阅读链接。该记录仍显示本机 MSVC STL 未满足 LWG3120 的初始缓冲重置要求，没有伪装成标准符合性通过。

审查工具边界：可用原生审查角色完成源码、构建、运行、JSON 控制复算；不可用的 LSP/AST 专用工具没有被冒称已执行。最早预设角色不可用时改用可用 code-reviewer；本交付不冒称 OMX CLI durable consensus。

## 最终教学反查及独立 good 修复

`c07_pipeline_review` 后续只读核对全局计划、重构指南、19 章、11 单元及 C08/C09/C10 反向入口。确认正文有实际机制推导，例如 IOCP 已接受请求生命期、SQE/CQE/部分提交、取消 accepted 集合、P1 区间契约、标准库源码差异与采样指纹，而不是只依据标题或链接批准。

该轮发现两项问题：L06/L09 good 与 Reference 字节相同（MEDIUM）；L05 观察入口容易被误读为 B01 正式采样入口（LOW）。作者重写两份 good：L06 使用独立 `good_detail`、handle/fd owner、attribute list 与进程编排；L09 使用独立 `unique_module` RAII 和符号解析。主代理修正文稿，区分 L05 observation/checker 与 B01 实际 resource baseline。

非作者再读 8 个相关文件，确认实现结构独立、未引用 Reference，Windows 实际运行两 good 和两 bad-rejected，4/4 PASS；两处文稿复读一致，返回 **APPROVE，0 未关闭问题**。最终 good 原始字节 SHA256 分别为 `99ac3eb7a1308c757cbe451399dead4adbf1de419fbd06f5e45de358cf7bb320` 和 `d54fa11b2441ae8e1f2a9cf095f93ce92e73664275431aa9b86a5b2f3d423163`。审查者输出的文本归一化 hash 与此不同，主代理用 `read_text()` 重算确认只是换行口径，冻结使用字节 hash。
