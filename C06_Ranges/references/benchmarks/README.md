# C06 benchmark evidence

`scripts/measure_b01.py` 负责调度 B01 独立进程、记录 raw stdout/stderr/json、保存 source/exe SHA256 和环境。正式采样必须等全课构建停止后再运行，避免和编译抢 CPU。

正式命令模板：

```powershell
python C06_Ranges\references\benchmarks\scripts\measure_b01.py --exe C06_Ranges\exercises\B01_cost\build-author\Release\B01_cost.exe --output C06_Ranges\references\benchmarks\results\b01-formal-YYYYMMDD-HHMMSS
```

默认协议：

- 1 轮 warmup，5 轮 sample。
- 固定 seed `42`。
- 日志流水线：小/大输入、低/高有效率，变体为 `loop`、`materialized`、`reparse`。
- 索引查找：小/大输入、低/高命中率，变体为 `linear_vector`、`sorted_vector`、`map`、`unordered_map`。
- 日志流水线和索引每个变体分别运行 `timed` 和 `counted` 两种 instrument。
- 每个样本必须 exit 0、无 timeout、输出 JSON 可解析、checksum/count 与进程内独立 oracle 一致，否则该组无效。
- driver 在所有进程结束后重新计算 source/exe/driver/process_runner SHA256；任一漂移会使报告状态为 `FAIL`。

作者 smoke 记录不是正式性能结果。
