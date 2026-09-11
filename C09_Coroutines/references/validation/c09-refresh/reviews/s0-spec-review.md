# S0 规格审查

审查日期：2026-09-11。

被审文件：`C09_Coroutines/references/implementation-spec.md`

- Git blob SHA: `62094f30049adfc613736dedbad6af854ad36690`
- SHA256: `84742569FA21E1FA20C8240CCE82D2E7BB94E869EF62A360DE0860169A61D2A9`

## 结论

APPROVE。

## 已核对要求

- 范围覆盖 37 个 C09 单元，采用按缺口增量修改，不全量重写；保留旧题号、Student 作业、Reference 和历史证据。
- 写入范围限定在 `C09_Coroutines/**`，根 `README.md` 与 `LEARNCPP_GLOBAL_PLAN.md` 仅允许更新 C09 导航和状态。
- 明确不安装系统组件、不升级依赖、不新增 CI，不涉及凭据、生产系统、提交或推送。
- 样章门禁明确：先以 G3 与共享 `lazy_task` 完成原失败复现、所有权修复、原复现复跑和非作者复验，通过后才批量深化。
- 已纳入已知阻断：`lazy_task` 移动结果异常路径 frame 所有权、`mini_ref::task_scope::spawn` 启动失败计数回滚、爬虫错误 URL 保存、H3 学生 `my_task` 三链检查、RPC Starter 与 Reference 协议统一、F3 性能证据降级。
- RPC 契约对齐现有 Reference：8 字节十进制长度头、4096 body 上限、`Q/C/R` 消息、`idempotent`、默认 `retries=0`、协议错误映射 `SerializationError`。
- Student/Reference 边界明确：Reference=OFF 隔离，未完成实现型起点必须安全有限并非零失败，独立 good 证明可做，行为型 bad 证明 checker 有效。
- 性能与前沿边界明确：取消固定倍率阈值，保留原始样本与中位数/范围；能力探针和主体测试分离，`DISABLED`、`SKIP`、`FAIL` 分开。
- WSL/liburing 决策明确：通过局部 `COROUTINE_STUDY_LIBURING_ROOT` 使用现有 liburing 2.15，queue_init 已实测成功后 I2/Linux 进入必测项。
- 最终验收明确：Windows Debug/Release、light 依赖路线、WSL、Student、生命周期、RPC、文档/实验矩阵；普通测试 30 秒、RPC 60 秒进程外超时；最终源码、文档、数据、命令和审查绑定版本。

## 非阻断注意

规格中 stdexec 与 Asio 固定版本写了 tag/分支名，最终质量报告仍应补写 exact SHA：`stdexec 6d7ad689`、`Asio 8806a680`，避免缓存版本绑定变弱。
