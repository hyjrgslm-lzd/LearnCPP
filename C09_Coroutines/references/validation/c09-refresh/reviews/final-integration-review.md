# C09 最终集成独立验收

日期：2026-09-11。

结论：APPROVE。

本次是最终清单字节回读与集成验收。只读核对最终矩阵、`source.json`、当前源 hash、`quality-report.md`、`coverage.md`、`implementation-spec.md`、`changes.md`、`delivery-manifest.json`、`delivery-audit.json` 与已有非作者 review 边界；不重跑全量测试。唯一写入是本文件，且本文件已被 delivery manifest 排除。

## 最终矩阵

- Windows 矩阵：`references/validation/c09-refresh/final/windows-20260911T050301Z/`
- Student 矩阵：`references/validation/c09-refresh/final/student-20260911T050302Z/`
- Linux 矩阵：`references/validation/c09-refresh/final/linux-20260911T050306Z/`

回读结果：

- Windows `summary.json`：verdict PASS；`cmake-version/configure/build-Release/ctest-Release/build-Debug/ctest-Debug` 全 PASS。
- Windows Release/Debug `ctest`：exit 0，均报告 65 个条目，尾部明确 `H2_std_task_probe (Skipped)`；最终口径为各 64 PASS + 1 probe SKIP。
- Student `summary.json`：verdict PASS；30 个 starter/观察目标按预期通过判定：7 个已提供实现/观察 PASS，23 个安全、明确、有限拒绝；另有 `cmake-version/configure/build-Release/test-list` 等配置/列举记录 PASS。`mini_task_sender_test` 为 exit 1 的预期 TODO 拒绝。
- Student `isolation.json`：`violations=[]`，`course_sources=51`，`source_reports=33`，`dependency_logs=21`，`missing_sources=[]`。符合 Reference=OFF 隔离构建 0 漏/0 Reference。
- Linux `summary.json`：verdict PASS；`cmake-version/compiler/kernel/configure/test-list/build-Release/ctest-Release` 全 PASS。
- Linux Release `ctest`：exit 0，50/50 PASS。`excluded.json` 明确 4 个 `<generator>` 相关目标因 GCC 13 libstdc++ 缺 `<generator>` 排除；`generator-capability.json` 为 SKIP，错误为 `fatal error: generator: No such file or directory`。

## Source 回读与后置格式化

三套最终矩阵 `source.json` 均为 196 个编译相关输入，路径分隔符归一后彼此一致。

`final/final-source-readback.json` 已更新为最终口径：三套矩阵均为 196 文件，其中 195 个当前字节一致，唯一差异为 `exercises/H2_std_execution_task/CMakeLists.txt` 的尾空行剥除；`unexpected_mismatches=[]`。

`final/post-matrix-formatting.json` 已回读：

```text
path = C09_Coroutines/exercises/H2_std_execution_task/CMakeLists.txt
before_sha256 = 72480397818bcbd293f85545e8ffe250919c137bb71bb3b230b2f0712efee558
after_sha256  = 482ace78f174d7730fd2aaca100edcfbeb803faebd2be640ee985fa9ced94f87
identical_without_trailing_newlines = true
```

矩阵中记录的 H2 before SHA 与该 before SHA 匹配，当前文件与 after SHA 匹配。后置 `post-format-configure-windows.json` 与 `post-format-configure-student.json` 均 verdict PASS，证明尾空行清理后 Windows RefON/RefOFF 配置仍可生成。

## Manifest / audit / ledger

`delivery-manifest.json` 最终 SHA-256：

```text
BD881C9FD6FCCAA10B89F591BFB148829676E5A9EE1C15BEE2202037D046638E
```

回读 manifest：

- `files` 条目 1035。
- `excludes` 条目 3：`delivery-audit.json`、`delivery-manifest.json`、`reviews/final-integration-review.md`。
- 本文件不在 `files` 中，且在 `excludes` 中；符合“最终审查文件被 manifest 排除”的自举边界。
- manifest 内 1035 个文件当前全部存在，当前 byte count 与 SHA-256 全部匹配。
- manifest 内无 `build/` 路径。
- manifest 内无 `.exe/.dll/.lib/.obj/.o/.a/.so/.pdb/.ilk/.idb` 等编译物扩展。
- manifest 内文件头扫描 MZ=0、ELF=0。

`delivery-audit.json` 已回读：

```json
{"documents":61,"local_links":1049,"units":37,"failures":[],"verdict":"PASS"}
```

`changes.md` 已回读：包含 `delivery-audit.json`、`delivery-manifest.json`、`final-source-readback.json`、`post-matrix-formatting.json`、`post-format-configure-student.json`、`post-format-configure-windows.json`、`quality-report.md`、`implementation-spec.md` 和本 `final-integration-review.md` 的最终收尾/明细条目。ledger 已覆盖本轮最终证明与审查文件范围。

## 规格、覆盖与质量报告

`implementation-spec.md` 已回读：S0-S6 全部为 `[x]`。

`coverage.md` 的 37 单元表自动计数为 37 行，覆盖：P1/P2、A1-A3、B1-B3、C1-C3、Capstone1、D1-D3、E1-E3、F1-F3、G1-G3、H1-H3、I1-I5、J1-J3、Capstone4、Capstone5。`delivery-audit.json` 独立报告 `units=37` 且 `failures=[]`。

`quality-report.md` 已回读并与最终证据一致：

- Student 起点、Reference、独立 good/bad、观察项和能力 SKIP 分开报告，不相加为“学生已完成 37 题”。
- source 回读口径已改为“196 个输入中 195 个当前 SHA 一致 + 1 个 H2 尾空行格式化差异”。
- F3 只保留同契约样本、IR/汇编/分配证据和边界判读；无倍率硬门槛，无性能排名。Windows 采样存在其他构建进程干扰，报告已明示。
- TSan 因本环境能力探针异常 memory mapping 作为 SKIP；Folly/Cobalt 保留内容与步骤但当前未运行。
- RPC `good/` 标为 Reference-adapted 答版；独立 protocol good 只证明 protocol Part。
- G1/shared_task/run_loop 生命周期边界不扩大声明到任意并发销毁。

## 非作者 review 边界

- `s0-spec-review.md`：S0 implementation spec 已 APPROVE，记录文件 SHA。
- `s1-review.md`：样章 lazy_task / scope 已有非作者复核，保留 source-before 差异边界。
- `runtime-review.md`：覆盖 lazy_task、sync_wait、run_loop、shared_task、when_any 等生命周期和 ASan 复验边界；明确不宣称任意线程安全销毁。
- `content-experiment-review.md`：H/F3/实验与 Student 检查边界独立审查通过。
- `mini-student-review.md`：原三类 mini good/bad 复跑通过；新增 task sender default/good/错值 bad/双完成 bad 通过同一 checker 验证；声明 good 只证明教学检查可做性。
- `rpc-review.md`：RPC public API、Student 安全 stub、同一 semantic checker、Reference 隔离、loopback/timeout/cancel/retry/drain 均通过非作者复验；明确 bad CTest 记录路径与审查手动记录来源不同。

## 最终判定

APPROVE。最终矩阵、source 回读、manifest 字节、audit、coverage、quality-report 和非作者审查边界一致。未发现阻断项。

本结论不声称我重跑了全部测试；全量测试结论来自已冻结矩阵文件的只读回读。
