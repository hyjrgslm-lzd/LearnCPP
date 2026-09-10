# B01 compile cost：编译成本观察

默认构建只运行有限正确性观察，不跑正式 benchmark。正式测量使用课程根目录下的 driver：

```powershell
python C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py --output-root C04_Generic_CompileTime_Reflection/references/benchmarks/results
```

本题有两组同契约对照。

第一组固定“查询目标类型是否在类型集合中”的契约，对 32、128、256 个类型比较递归 traits 与折叠表达式。正确性只证明两者答案相同；编译成本必须另看 trace、实例化计数和正式计时样本。

第二组固定“4 个翻译单元调用同一个模板实体”的契约，比较每个 TU 隐式实例化与 `extern template` 加一个显式实例化定义。对象文件里的 `.text` 代表链接前编译产物，最终 exe 的 `.text` 才能观察链接后的代码体积；两者不能混用。

正式计时口径：一次预热、五次独立进程样本，固定随机种子安排版本顺序。trace 插桩与正式计时分开运行。

本机正式结果见 `../../references/benchmarks/results/latest-summary.md`，原始目录为 `../../references/benchmarks/results/compile-cost-run-20260910-004959/`。本机 `time.monotonic()` 分辨率为 0.015625s；表格保留 4 位小数用于复算，不表示能从 1ms 差异推断收益。该结果显示：32 项和 128 项类型查询本次未分辨出稳定收益，不能证明性能相等；256 项折叠版本中位数低于递归版本。多 TU 显式实例化减少调用方 `.obj` `.text`，但最终 `.exe` `.text` 不变，本轮编译时间中位数更慢；它由 4/5 个短编译进程耗时相加，解释更要谨慎。

扩展组比较手写递归 type map 查找与 Boost.Mp11 `mp_map_find`。规模为 32、128、256 个键；查询最后一项和缺失项；manual/Mp11 共 12 组。manual 与 Mp11 源文件使用同一个 `source_map`、同一个 `entry_value` 输出适配、同一 Boost.Mp11 头依赖、同一 include path、同一编译选项，只隔离查找机制。依赖必须由 `../build/_deps/mp11-boost-1.91.0/.learncpp-dependency.cmake` 指向 Boost.Mp11 commit `b94b089d4ec83cd397f20958f34edf25bc3e06f4`，Git HEAD 必须相同且工作树必须干净，否则 driver 失败。

扩展组有限正确性观察：

```powershell
python C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py --output-root C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/cost-meta-map-checks --meta-map --check-only
```

扩展组正式成本实验必须等独占测量窗口：

```powershell
python C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py --output-root C04_Generic_CompileTime_Reflection/references/benchmarks/results --meta-map
```

`--meta-map` 输出目录名为 `meta-map-run-<timestamp>`，不会覆盖旧 `latest-summary.md`。正式结果使用 `time.perf_counter()` 包装现有有界 runner；计时包含 Python runner、进程创建、clang-cl 编译和等待返回，源码/产物哈希在计时外记录。
