# F-3 HALO 触发与失败诊断

对应正文：[08 模块 F](../../08-模块F-协程帧与allocator.md#f3)。

本练习诊断 coroutine state 的 heap allocation elision。重点是用编译器证据说明分配有没有被省略；计时只作为线索。

## Part 1：两个版本先跑通

版本 A 局部创建并消费 generator，返回对象和 handle 不逃逸。版本 B 故意把对象地址写入全局，给优化器制造候选逃逸路径；这不保证阻止分配消除。

两版契约相同：输入 `n`，返回 `0 + ... + (n - 1)`。版本 A 局部创建并消费 generator；版本 B 先把 generator 对象地址写入全局，再按同样方式消费。先运行程序确认两版计算结果一致。性能计时中不要打印每个元素，I/O 会淹没分配差异。

## Part 2：用工具链证据判断

可选诊断：

在本题目录、已有相应编译器的 shell 中执行。本机 Clang 18 位于 WSL 的 `/usr/lib/llvm-18/bin/clang++`：

```bash
mkdir -p ../build/f3-diagnostics
/usr/lib/llvm-18/bin/clang++ -std=c++23 -O2 -I../include -DF3_UNINSTRUMENTED_HALO -Rpass=coroutine-elide -S -emit-llvm main.cpp -o ../build/f3-diagnostics/f3.ll
g++ -std=c++23 -O2 -I../include -fdump-tree-all -c main.cpp -o ../build/f3-diagnostics/f3.o
```

MSVC 可在 Release 反汇编里找分配调用。记录编译器版本、优化级别、实际 consumer 调用路径中是否看到 `operator new` 或 elide remark。独立导出的协程工厂仍含分配，不代表已内联的 consumer 也分配。需要排除 allocation hook 扰动时，用同一源码加 `-DF3_UNINSTRUMENTED_HALO` 生成 IR 或汇编。

## Part 3：小型采样

`sample_f3.py` 先对每版执行一个独立预热进程（1000 iterations），随后各取 5 个独立进程样本；固定 seed 打乱版本顺序，输出所有原始样本、median、min/max。被测程序不再隐藏额外预热。`steady_clock` 计时区间内累计 checksum，结束计时后验证总量，不在计时内打印日志。

参数必须是完整正整数，`n <= 4096`、`iterations <= 10000000`、总元素数不超过一亿，保证计数不溢出并限制工作量。`--bench local 32 2` 的 sink 必须为 992；它只用于检查数据口径，不能拿两个 iterations 得出性能结论。普通 `F3_halo_contract` CTest 会运行与 benchmark 共用的 A/B 正确性检查，原 Reference 另保留 hook 生命周期观察；两者都不单独证明 HALO。

```powershell
python C09_Coroutines/exercises/F3_halo_diagnose/sample_f3.py C09_Coroutines/exercises/build/c09-author-mechanisms/F3_halo_diagnose/Release/F3_halo_diagnose.exe
```

如果 `local` 没有更快，或者两组都接近，也是真实结果。不能按预设倍率改结论。

## Part 4：分清三种结论

- “程序更快”只说明可能发生优化。
- “remark 显示 frame elided”说明当前 Clang 在此处做了 HALO。
- “汇编中没有分配调用”说明当前生成代码省掉了 heap allocation。

标准只允许实现省略分配，不保证任何具体代码形状一定触发。

## 验收

- 记录 A/B 两版同契约正确性和耗时样本。

  **答案解析：** A/B 都计算同一个求和结果。耗时只能作为线索：局部创建并消费的版本若明显更快，可能更利于 HALO 或内联；逃逸版本通常让优化器更难证明生命周期嵌套。时间受机器、优化级别、计时粒度和 I/O 影响，所以只记录本机样本和条件，不把耗时当最终证明。
- 记录至少一种编译器诊断证据。

  **答案解析：** HALO 的可靠证据要来自编译器输出，例如 Clang `coroutine-elide` remark、GCC coro dump、MSVC Release 反汇编，或分配调用是否消失。F-3 starter 的 allocation hook 是插桩观察，可能扰动优化；必须与无插桩 IR/反汇编/remark 分开记录。诊断证据要写清编译器版本和优化级别。
- 列出三种破坏 HALO 的模式：返回对象逃逸、handle 逃逸、跨编译单元不透明消费等。

  **答案解析：** HALO 依赖实现能证明 coroutine state 生命周期严格嵌套在调用方内，且大小在调用点可知。返回对象逃逸、保存裸 `coroutine_handle`、跨编译单元/虚调用隐藏消费路径，都会让这个证明变困难或失败。F-3 的版本 B 只是候选逃逸对照；优化器可能证明地址写入不影响最终结果，仍消除分配。
- 解释 HALO 处理 frame allocation，E-3 ready 短路处理 await-expression 控制流。

  **答案解析：** HALO 只讨论整个 coroutine state 的 heap allocation 是否省略；触发后语义仍像存在 frame 一样。ready 短路讨论单个 `co_await` 的控制流，`await_ready()==true` 时跳过 `await_suspend()`。一个协程可以有 ready 短路但仍分配 frame，也可以有真实挂起点但在某个实现上被 elide。

## Reference

Reference 是稳定可编译 fixture；通过 compiler remark/dump/disassembly 判断是否 elide heap allocation，不能用微基准时间当作唯一证明。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target F3_halo_diagnose F3_halo_diagnose_reference
ctest --test-dir build/dg-lane -C Release -R F3_halo_diagnose_reference
```

## 本轮实际结果

Clang 18.1.3、`-O2` 无插桩 IR 中，local/escaped 两个 consumer 都没有动态分配，均被化简为求和循环；独立 `range_values` 仍分配 48 字节。没有 elide remark，不能指定优化 pass。两版循环代码不同，本机耗时差不能归因于“只有一版 HALO”。完整原始样本、IR/汇编、指纹及并行构建干扰说明见[最终实验判读](../../references/validation/c09-refresh/performance/final/analysis.md)。
