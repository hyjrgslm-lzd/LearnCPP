# L10 clocks and durations

目标：观察 `system_clock`、`steady_clock` 与 `duration` 转换边界。编辑入口无；本题是观察型。

Part 1：确认 `steady_clock::is_steady` 与 `system_clock` 不是同一个教学概念。`system_clock` 给 UTC 时间线附近的 wall time，`steady_clock` 用于测量间隔。

Part 2：比较 `duration_cast`、`floor`、`ceil`、`round`。`2500us` 转 `ms` 时，截断、向上取整与四舍六入五成双规则不同。

Part 3：检查 `duration` 的表示范围。范围来自 rep 和 period，不来自变量名。需要先判断能否表达，再做窄化或计数转换。

运行：

```powershell
cmake -S L10_clocks_durations -B build/leaf-L10 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L10 --config Release --parallel 2
ctest --test-dir build/leaf-L10 -C Release --output-on-failure
```

解析：通过只说明这些检查在当前进程里成立；它不证明 `system_clock` 永不调整，也不把 `steady_clock` 的 epoch 变成可序列化数据。序列化时间请用显式 UTC Unix 毫秒，显示时再选时区。
