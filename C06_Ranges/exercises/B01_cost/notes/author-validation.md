# B01_cost 作者验证记录

范围：

- `C06_Ranges/exercises/B01_cost/**`
- `C06_Ranges/references/benchmarks/**`

初版未修改 CAPSTONE1 样章、CAPSTONE1 代码或公共 helper；r3 按 review 授权仅补 CAPSTONE1 collector 不计数 overload。

## 构建与快测

```powershell
cmake -S C06_Ranges\exercises\B01_cost -B C06_Ranges\exercises\B01_cost\build-author
```

结果：exit 0。生成器为 Visual Studio 18 2026，编译器为 MSVC 19.51.36256.0。

```powershell
cmake --build C06_Ranges\exercises\B01_cost\build-author --config Release --target B01_cost
```

第一次结果：exit 1。`std::ranges::lower_bound` 要求 comparator 满足 `indirect_strict_weak_order`，补了 `CountedLess::operator()(int, int)` 后重跑。

最终结果：exit 0，生成：

```text
F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe
```

```powershell
ctest --test-dir C06_Ranges\exercises\B01_cost\build-author -C Release --output-on-failure
```

结果：exit 0。`B01_cost_check` 通过。

```powershell
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --check
```

结果：exit 0，输出：

```text
B01_cost checks OK
```

## Reference 选项关闭

```powershell
cmake -S C06_Ranges\exercises\B01_cost -B C06_Ranges\exercises\B01_cost\build-author-reference-off -DRANGES_BUILD_REFERENCE=OFF
```

结果：exit 0，输出包含：

```text
-- B01_cost skipped because RANGES_BUILD_REFERENCE=OFF
```

```powershell
ctest --test-dir C06_Ranges\exercises\B01_cost\build-author-reference-off -C Release --output-on-failure
```

结果：exit 0，输出：

```text
No tests were found!!!
```

这是显式选项关闭，不是能力 SKIP。

## 非正式 smoke

未跑正式 1+5 全矩阵采样。以下只验证 driver、JSON、checksum、source/exe hash 和进程外 timeout 管线。

```powershell
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-smoke-author-r1 --smoke --samples 1 --warmups 1 --timeout 30
```

结果：exit 1。本机 Python 3.10 没有 `hashlib.file_digest`。改为手动分块 SHA256。保留失败目录。

```powershell
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-smoke-author-r2 --smoke --samples 1 --warmups 1 --timeout 30
```

结果：exit 1。smoke schedule 只有 `loop` warmup，没有 `loop` sample，summary 把该组判为 invalid。补 `loop` sample。保留失败目录。

```powershell
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-smoke-author-r4 --smoke --samples 1 --warmups 1 --timeout 30
```

结果：exit 0：

```json
{
  "status": "PASS",
  "output": "F:\\CPPTrain\\LearnCPP\\C06_Ranges\\references\\benchmarks\\results\\b01-smoke-author-r4",
  "formal": false
}
```

## 正式采样命令

等 root 确认所有构建停止后运行：

```powershell
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-formal-YYYYMMDD-HHMMSS
```

默认协议是 1 warmup + 5 sample、seed 42、`N=256/8192`、有效率或命中率 `10/90%`。作者 smoke 结果不是正式性能结论。

## SHA256

```text
3A83D59D4DF12401595A4DE604B8599929A1CF782D698940FBFAB663BF2F1129  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\CMakeLists.txt
D620A10F3B55E1F09BBD5FDCC1CFE36932C6F66706C88B5B9C7B9A70B5D1940C  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\main.cpp
714798108F882CD21053AC9E52484D9BB3D32C9325C7F9E7F028CF8147969B86  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\README.md
5F57D8FB1E2B074C7752321EBBB0B04CF2A62306D33C3CA2D92B23BA776CDEDC  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\README.md
C8111A380C46CB4E62867FFB52A67305E0C795CC0404E87073F356B9B20E6F11  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\scripts\measure_b01.py
15117BF300EBFED896543273C709DA7CCEC958D276F01F91E3246451A63148FA  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe
A0137BCC1F1EDD583E9D644FC921FD622998CC4E3401A81D2ACECB4C837EA9F1  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\results\b01-smoke-author-r4\report.json
```

## 边界

- 没有正式性能样本。
- 没有从总耗时推断 cache miss、分支预测或分配器根因。
- 索引实验是整数键机制分支，不是 CAPSTONE1 四字段日志 schema 的改动。

## r3 BLOCK 修复验证

本轮修复 benchmark-review 的三项 BLOCK，仍未跑正式采样。

### 代码修复

- B01 pipeline 增加 `instrument=timed|counted` 分支。`counted` 调用 CAPSTONE1 带 `OperationCounts&` 的 overload；`timed` 调用不计数 overload，计时区间内不递增 `parse_attempts`。
- CAPSTONE1 四个 `log_pipeline.hpp` 增加不带 `OperationCounts&` 的 collector overload，默认 checker 计数语义不变。
- B01 每个 `--bench-one` 进程从生成时的语义字段构造独立 pipeline oracle，checksum 覆盖 `timestamp/level/user_id/message`；索引从原始 records/queries 用独立线性 oracle 校验。
- Python driver 删除“随机首个变体作 baseline”的判定，改为校验 payload 内的 `oracle_*` 字段。
- Python driver 开跑前后分别 hash B01 source、CAPSTONE1 source、exe、driver、process_runner；任一 drift 使报告 `FAIL`。新增 `--drift-self-check`。

### B01 最终快测

```powershell
cmake --build C06_Ranges\exercises\B01_cost\build-author --config Release --target B01_cost
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --check
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --drift-self-check
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-smoke-author-r6 --smoke --samples 1 --warmups 1 --seed 42 --timeout 30
```

结果：exit 0。输出包含：

```text
B01_cost checks OK
B01 drift self-check OK
```

```json
{
  "status": "PASS",
  "output": "F:\\CPPTrain\\LearnCPP\\C06_Ranges\\references\\benchmarks\\results\\b01-smoke-author-r6",
  "formal": false
}
```

补充读取 `b01-smoke-author-r6/report.json`：`status=PASS`，`hash_drift={}`，存在 `source_sha256_after`、`capstone1_source_sha256_after`、`exe_sha256_after`、`driver_sha256_after`、`process_runner_sha256_after`。sample 中 pipeline `loop/timed parse_calls=0`，`loop/counted parse_calls=256`，所有 sample 的 `checksum == oracle_checksum`。

```powershell
ctest --test-dir C06_Ranges\exercises\B01_cost\build-author -C Release --output-on-failure
```

结果：exit 0，`1/1 Test #1: B01_cost_check ... Passed`。

### B01 bad input

```powershell
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --bench-one --case pipeline --scenario small_low_valid --variant loop --instrument counted --n 16385
```

结果：exit 1，输出：

```text
B01_cost error: n must be in 1..16384
```

```powershell
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --bench-one --case index --scenario small_low_hit --variant unordered_map --instrument counted --rate 101
```

结果：exit 1，输出：

```text
B01_cost error: hit rate must be in 0..100
```

```powershell
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-bad-driver-r2 --samples 0 --warmups 1 --timeout 30
```

结果：exit 2，输出包含：

```text
measure_b01.py: error: samples, warmups and timeout must be positive
```

### CAPSTONE1 回归

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_reference
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_validation_good
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_validation_bad
ctest --test-dir C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author -C Release --output-on-failure
```

结果：exit 0，3/3 通过：`reference`、`validation_good`、`validation_bad_rejected`。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_student
C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author\Release\CAPSTONE1_log_pipeline_student.exe
```

结果：build exit 0，student 运行 exit 1，输出：

```text
check failed: valid row parses
```

### r3 指纹

```text
3A83D59D4DF12401595A4DE604B8599929A1CF782D698940FBFAB663BF2F1129  C06_Ranges/exercises/B01_cost/CMakeLists.txt
CCD8B22CA9423BD1501D8D93BB030E46B05A64E3DC8626380E117022BA4F7F88  C06_Ranges/exercises/B01_cost/main.cpp
6115352626B172DF4BB20131B925AB1840D41C4C48368FC02D9478024DCE2DA1  C06_Ranges/exercises/B01_cost/README.md
955AB303BF299BFF9E125A5988D12D006FFB84D8671328079B7276710712CE5A  C06_Ranges/references/benchmarks/README.md
B9B4518B70C66553CD0DCC2B3F0D0542372D74BE518099EA39CE4CF39ADF52D8  C06_Ranges/references/benchmarks/scripts/measure_b01.py
E743935111206DFF3E292AA4BF48591256ADACD5043624E7F2BAF7AB511D2B23  C06_Ranges/exercises/CAPSTONE1_log_pipeline/src/reference/log_pipeline.hpp
DE4FC02B195FC552E9AF0FF8614E4069F967198B6E6AF93F38236ED482644EE5  C06_Ranges/exercises/CAPSTONE1_log_pipeline/validation/good/log_pipeline.hpp
5446C055F76CDB13EEC14E0277C5135332E6AD11A6903E8F337B315DEA568D21  C06_Ranges/exercises/CAPSTONE1_log_pipeline/validation/bad/log_pipeline.hpp
775A4F272B64B51A1CD6268C357A0332C926B0CA25D91F09130E0D49F7A2D492  C06_Ranges/exercises/CAPSTONE1_log_pipeline/src/student/log_pipeline.hpp
0EA57756290798170EF0E42300D82B432AAB586C2818A5BEEC0E057094D0328A  C06_Ranges/exercises/B01_cost/build-author/Release/B01_cost.exe
0AC2E71A2D39C2455830499E035CFFA2F3D80A94348FB61B6377A79A0BD7FA02  C06_Ranges/references/benchmarks/results/b01-smoke-author-r6/report.json
```

