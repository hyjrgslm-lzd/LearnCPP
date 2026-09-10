# B01：Ranges 成本与索引查找基准

本题是性能切片入口。日志流水线复用 `CAPSTONE1_log_pipeline/src/reference/log_pipeline.hpp` 的 `parse_line`、循环收集和实体化 ranges 收集；计时路径调用 CAPSTONE1 的不计数 overload，计数路径调用原有 `OperationCounts` overload。索引查找是独立整数键机制实验，不改变 CAPSTONE1 的四字段日志 schema。

## 快速检查

```powershell
cmake -S C06_Ranges\exercises\B01_cost -B C06_Ranges\exercises\B01_cost\build-author
cmake --build C06_Ranges\exercises\B01_cost\build-author --config Release --target B01_cost
ctest --test-dir C06_Ranges\exercises\B01_cost\build-author -C Release --output-on-failure
```

`RANGES_BUILD_REFERENCE=OFF` 时本题不建 target。这是显式选项关闭，不是工具链能力 SKIP。

## 单进程入口

默认和 `--check` 执行正确性快测：

```powershell
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --check
```

单场景单变体采样：

```powershell
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --bench-one --case pipeline --scenario small_high_valid --variant materialized --instrument counted --seed 42
C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --bench-one --case index --scenario small_high_hit --variant unordered_map --instrument counted --seed 42
```

尺寸上界是 `N=16384`，查询量等于 `N` 且不超过 `N`。默认场景使用 `N=256/8192`，有效率或命中率为 `10%/90%`。

## 结果边界

计数版和计时版分开运行。计数版记录 `parse_calls`、比较次数、hash 次数和 equal 次数；计时版记录同一算法函数在少插桩路径下的耗时，日志流水线的 `parse_calls` 为 0，避免把计数写入计时区间。计数能说明调用次数，不能推出 cache miss、分支预测或分配器根因。每次 `--bench-one` 都和本进程独立 oracle 比对，日志 checksum 覆盖 `timestamp`、`level`、`user_id`、`message`。
