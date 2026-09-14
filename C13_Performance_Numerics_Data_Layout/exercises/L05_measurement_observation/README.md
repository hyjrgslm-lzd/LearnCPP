# L05：阶段计时观察

本程序运行固定输入并输出阶段 CSV。它只证明“生成、更新、归约”三个计时窗口被分开，且计时后做了轻量正确性检查；它不证明哪个阶段的根因已经确定，也不证明当前机器上的任何优化优越。

默认输入有限：`--size 10000 --seed 42 --steps 1`。`generate` 包含随机数与 SoA 构造；`update` 只包含位置更新循环；`reduce` 只包含动能归约。校验在计时后执行，不把失败样本写成性能结论。

```powershell
cmake -S C13_Performance_Numerics_Data_Layout/exercises/L05_measurement_observation -B build/c13-l05 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/c13-l05
ctest --test-dir build/c13-l05 --output-on-failure
.\build\c13-l05\C13_L05_measurement.exe --size 10000 --steps 3
```

读结果时先看 `variant`。如果 `generate` 很大，只能说明随机输入和构造在这个窗口里耗时；如果 `update` 变化，仍需反汇编、向量化报告、硬件计数器或受控对照才能把原因归到 SIMD、cache、branch 或 allocator。总时间不是根因。

这是二维、含质量与动能的独立观察分支；L04 三维主项目返回坐标与稳定和，两者不可直接混排计时。多步更新用 double 标量表达式作为 oracle，并按更新次数推导浮点误差上界；不能用只适合一步的固定微小容差判断一千次 float 累加。

## 分配、分支和步长对照

同目录的 `costs.cpp` 生成 `c13_costs`，默认 `--size 4096`，最多 65536。它执行三个短实验：

- 用实际 `std::pmr::memory_resource` 统计成功分配次数和请求字节，比较逐步增长与预留容量。计时包括 reserve/fill，不含 vector 销毁；计数器属于插桩成本，不能当作无插桩 allocator 的最终排名。
- 调用真实 C08 scalar/SSE2/可用 xsimd 的 ReLU，对 NaN、负数、负零与尾部检查相同声明语义，输出先填错误哨兵。
- 对同一行优先整数值矩阵分别按行、按列遍历，检查相同总和。这里观察的是访问顺序，未采集实际 cache miss。

命令：`cmake --build build/c13-l05 --target c13_costs`，随后 `./build/c13-l05/c13_costs.exe --size 4096`。在完整预设中该目标也参与短检查。原理、口径与自测答案见 [成本章节](../../chapters/12-cache-branches-allocation.md)。
