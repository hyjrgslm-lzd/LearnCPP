# C07 最终证据复审

角色：最终交付证据的独立 verifier。范围除本文外只读。本轮没有构建、编译、运行 benchmark，也没有修改源码。

## 结论

APPROVE，针对本轮已复审的证据集。

最终交付 manifest 冻结和 byte readback 不在本轮范围内；父侧写入最终 manifest 后还需要单独核证。

## 核查方法

- 读取 `references/quality-report.md`、`references/measurements/cost-analysis.md`，以及它们引用的 validation/measurement JSON 与 JUnit artifact。
- 从 `ctest.json` stdout 和可用的 `ctest.xml` 复算 CTest 数量。
- 从 `student-verification.json` 复算 Student 隔离数量。
- 直接从全部 sample JSON 复算 B01 raw/warmup/formal 数量，以及每个 case 的 median/min/max/sample-stdev；不把 `arithmetic-audit.json` 当作唯一证据。
- 从当前本机字节复算 Windows B01 executable/source fingerprints。
- 在 WSL 内复算 Linux B01 executable/source fingerprints，对照 `/root/learncpp-c07/snapshots/final-linux-release-r1` 和当前 workspace 字节。
- 遍历三个旧 Linux snapshot manifest 里列出的全部文件，比较当前 snapshot 字节 hash 与 manifest hash。
- 核查 MSVC LWG3120 observation 是否被报告为标准符合性 FAIL，而不是藏在 CTest PASS 里。

## 矩阵证据

整课 CTest 证据与 `quality-report.md` 一致：

| Evidence | 复算结果 |
|---|---:|
| `final-windows-release-r1` | 48 tests, 0 failures, 0 skipped, 0 errors |
| `final-windows-debug-r1` | 48 tests, 0 failures, 0 skipped, 0 errors |
| `final-windows-asan-r1` | 48 tests, 0 failures, 0 skipped, 0 errors |
| `final-linux-release-r1` | 48 tests, 0 failures, 0 skipped, 0 errors |
| `final-linux-debug-r1` | 48 tests, 0 failures, 0 skipped, 0 errors |
| `final-linux-asan-r1` | 48 tests, 0 failures, 0 skipped, 0 errors |
| `final-linux-core-r1` | 42 tests, 0 failures, 0 skipped, 0 errors |

Student 证据与 `quality-report.md` 一致：

| Evidence | 复算结果 |
|---|---:|
| `final-windows-student-r1/student-verification.json` | PASS, 10 targets, 10 controlled rejections, audit PASS, 2693 include records |
| `integration-student-r2/student-verification.json` | PASS, 10 targets, 10 controlled rejections, audit PASS, 3658 include records |

增量 good 证据与 `quality-report.md` 一致：`good-independent-r2/evidence-summary.json` 记录 6 个环境、每环境 2 个 target，共 12 条 PASS test records。没有记录出现非零 exit、timeout 或 FAIL verdict。

## 叶级证据

独立叶级证据与最终版本边界一致：

| Evidence | 复算结果 |
|---|---:|
| `leaf-verification-20260910T225908/windows/summary.json` | 11 leaf configure/build/CTest PASS, 47 tests, 0 failures |
| `leaf-verification-20260910T150538Z/linux/summary.json` | 9 stable leaf configure/build/CTest PASS, 40 tests, 0 failures |
| `leaf-verification-20260910T151328Z/linux-l06-l09/summary.json` | fresh delta snapshot, L06/L09 configure/build/CTest PASS, 7 tests, 0 failures |
| `leaf-verification-20260910T231413/windows-l06-l09/summary.json` | L06/L09 Windows revalidation PASS, 7 tests, 0 failures |

报告没有把旧 r1 整课 48-test artifact 写成 L06/L09 good rewrite 之后的全量重跑。最终边界写法是 old r1 matrix + 12 incremental good tests + new two-leaf evidence。

## 源码与快照边界

当前两个 rewritten good 文件的字节 hash 与 Reference 文件不同，并与报告一致：

| File | SHA-256 |
|---|---|
| `L06_process_ipc/validation/good/process_ipc.hpp` | `99ac3eb7a1308c757cbe451399dead4adbf1de419fbd06f5e45de358cf7bb320` |
| `L06_process_ipc/src/reference/process_ipc.hpp` | `7e2d6ba0094db4b77a5bf66c845bfbde66699286446c5f35035713d1ad875283` |
| `L09_dynamic_loading/validation/good/dynamic_loading.hpp` | `d54fa11b2441ae8e1f2a9cf095f93ce92e73664275431aa9b86a5b2f3d423163` |
| `L09_dynamic_loading/src/reference/dynamic_loading.hpp` | `5f442e76be43679e44f30707295d4e2940f479307ccd838041b80c4eaf509058` |

我遍历了每个旧 Linux snapshot manifest 的 131 个文件：

| Snapshot | Changed from manifest | Missing files | 结果 |
|---|---:|---:|---|
| `final-linux-debug-r1` | 2 | 0 | 仅 L06/L09 good changed |
| `final-linux-release-r1` | 2 | 0 | 仅 L06/L09 good changed |
| `final-linux-asan-r1` | 2 | 0 | 仅 L06/L09 good changed |

这与 `good-independent-r2/snapshot-delta.json` 一致。报告也明确说旧 manifest 不再代表这些目录的 frozen-current 状态。

## B01 测量证据

从 sample JSON 独立复算结果与报告和 `arithmetic-audit.json` 一致：

| Phase | Raw | Warmup | Formal | Valid formal | Cases |
|---|---:|---:|---:|---:|---:|
| Windows baseline | 30 | 5 | 25 | 25 | 5 |
| Windows compare | 102 | 17 | 85 | 85 | 17 |
| Linux baseline | 30 | 5 | 25 | 25 | 5 |
| Linux compare | 102 | 17 | 85 | 85 | 17 |
| Total | 264 | 44 | 220 | 220 | 44 |

每个 B01 case 都从 5 个 formal samples 复算 median、min、max 和 sample standard deviation；结果在浮点容差内匹配对应 `result.json` summary。所有 formal samples 都满足 `verdict=VALID`、`exit_code=0`、无 timeout、无 supervisor error、无 cleanup error、`temporary_directory_removed=true`、parsed `valid=true`。

Fingerprint 检查：

| Platform | 结果 |
|---|---|
| Windows baseline/compare | 当前 executable hash 匹配 `c557ef164161acf5161ffd32217bf0669d931a951a68e2454d9be1346fe2a992`；13 个 source hash 匹配当前 workspace bytes；phase start/end fingerprints 一致 |
| Linux baseline/compare | 当前 executable hash 匹配 `639cf402833c15c42081e27e91e6ee0d0f08fbffcbc1282f38e79070a2ab368d`；13 个 source hash 同时匹配 release snapshot 与当前 workspace bytes；phase start/end fingerprints 一致 |

`cost-analysis.md` 保持在数据范围内：不做跨平台排名，不声称统计显著，不声称生产优化，不把 reader 单独归因。表格与我复算的 phase 数据一致。

## LWG3120 边界

`source-observation-windows-r2.json` 记录：

```json
{"standard_expected_reset":true,"observed_reset":false,"second_allocation_bad_alloc":true,"fresh_scope_control":true}
```

`quality-report.md` 明确说这是 Windows 标准符合性检查 FAIL，CTest PASS 只表示 observation 被成功捕获。我没有发现 MSVC LWG3120 failure 被计为标准 PASS 的证据。

## 最后窄复读

`cost-analysis.md` 的四处精确澄清已复读：

- 4 KiB completion depth 说明已修正为每轮只有 1 个 chunk，两轮累计 2 次请求但不会并发，所以 peak 只能到 1。
- `app_copy` 已限定为 reader 为产生 owned chunks 做的显式物化复制，不包含 P1 assemble、通用容器复制、输入初始化或内核内部复制；buffered 的 0 不代表全链路零复制。
- Linux standard pmr 结论已写成 counters 证明资源策略差异，但本次总时间未优于手写定长 pool。
- Linux completion 的 max 尖峰只记录为现象，明确说调度和后台干扰未控制、尖峰原因尚未定位，没有强行归因。

列说明也已澄清：阶段列和计数列均为 5 个正式样本的 median，阶段 median 相加不保证等于总时间 median。temp 文件系统链接已存在并可读：`filesystem-windows-temp.json` 记录 Windows temp 为 NTFS，`filesystem-linux-temp.json` 记录 `findmnt -T /tmp` 为 ext4。

窄复读结论：无新增问题，原 APPROVE 结论不变。

## 缺口

- Final delivery manifest freeze 和 byte readback 在本轮期间尚未完成；按父侧指令，这不是本轮否决项。
- 本轮没有重跑 build、test 或 benchmark。

## 风险

- 整课 r1 矩阵仍是 L06/L09 good rewrite 前的 historical baseline artifacts。最终声明必须保持已写明的边界：r1 matrix + good-independent-r2 increment + fresh L06/L09 leaf validation。
- B01 结论只适用于这台机器、这套协议、每 case 5 个 formal samples，以及 baseline/compare 非交错 phase。
