# C07 验证工具的非作者审查与闭环

审查者：原生 code-reviewer 子任务 `c07_architecture_fallback`。作者为主代理与 `c07_delivery_patterns_fallback`，不是审查者。本文由主代理归档审查返回的事实，不把作者自检替代非作者复验。

## 初次发现

1. 子 agent 受限执行 `check_runner.py` 时，timeout 子进程的 taskkill 返回 Access denied。父代理没有放宽 cleanup 条件，而是在本任务已授权的正常本机执行权限中复跑严格校准，exit 0。审查者回读 [runner-windows-initial.json](runner-windows-initial.json) 后确认权限边界，并关闭该阻断。
2. `audit_delivery.py` 的 `summarize_records()` 原先只计数 verdict，未把 FAIL/UNKNOWN 纳入总 failures。审查者用临时 course 和一条 `{"verdict":"FAIL"}` 实测，工具错误返回 0/PASS。这是真实代码缺陷。

## 修复与非作者复验

工具作者将 FAIL、缺失/未知 verdict、损坏 JSON 加入 failures；失败记录带路径、name、exit_code、timeout，损坏 JSON 另带解析错误。self-check 增加上述三种反例，SKIP 只计数，不自动批准必需矩阵。CLI 明确只传本批审核记录目录，不混扫历史失败。

审查者重跑原来的临时 course + FAIL record 复现，确认 exit 1；重跑 `audit_delivery.py --self-check` 通过。结论 **APPROVE**，原阻断已关闭。

## 其余审查范围与证据

- `implementation-spec.md`、`StudySetup.cmake`、`run_test.py`、`check_runner.py`：独立 Student/Reference、精确反例退出、真实 timeout/cleanup FAIL、默认离线路线成立。
- `prepare_uring.sh` 和原生 CMake 前缀接线：不装系统包，固定 liburing 2.15 输入；[uring-setup.json](uring-setup.json) 记录构建 PASS 和源/版本回读。
- `snapshot_linux.py`：只在独立 guest ext4 快照内复制本课与必要 C01/C02 输入，排除 C08、构建产物和缓存，不覆盖已有快照。
- `verify_students.py`：复用原有 codemodel/include 审计，Student 必须 exit 1 + checker 诊断且没有 timeout/error/cleanup_error。
- `record_environment.py`：记录明确工具、时钟、平台与选定文件 hash，不转储用户环境变量。
- `run_linux_matrix.sh`：在 guest 内固定 PATH，snapshot/build/evidence 分离，每步有界记录，失败时导回本批证据，不操作 C08。
- 审查者抽查 [foundation-r2](foundation-r2/ctest.json)：配置/构建/CTest PASS，6 条测试记录没有 timeout/cleanup_error；snapshot manifest 为 51 文件，ext4 检查通过。

本记录只批准上述版本的工具，不批准尚未完成的课程正文、后续代码或最终矩阵。后续修改仍须针对变动复验；最终工具/内容版本以交付 manifest 对应。

收尾接线补证：L01 新增线程观察后，`foundation` 的显式构建目标列表也加入 `L01_thread_process`，使 `^L0[12]_` 测试集合都有对应二进制。[foundation-r4](foundation-r4/ctest.json) 重新配置、构建、运行 8/8 通过。此处为主代理接线复验，不冒称额外非作者实跑。
