# B01：调用粒度与额外复制的受控实验

先读[成本实验](../../chapters/22-boundary-cost.md)。这是完整观察程序，不是已替学生完成的实现题。所有模式使用同一个 `core/c18/bytes.hpp`；不把三份不等价算法放在同一排行榜。

## Part 1：建立并解释基线

`per_record` 对每条记录通过函数指针调用一次转换内核，统计实际成功调用次数和完成字节数。输入总量固定64 KiB，每轮处理完整输入。分别取16、1024、65536字节记录，预测调用数如何变化，然后运行程序。

```sh
cmake -S exercises/B01_boundary_cost -B build/b01
cmake --build build/b01 --config Release
ctest --test-dir build/b01 -C Release --output-on-failure
python tools/run_costs.py --exe build/b01/Release/c18_boundary_cost.exe --output build/costs/baseline.json
```

单配置生成器程序不在 Release 子目录。runner 为每个场景预热一次、启动五次独立进程，以固定种子交错场景顺序，保留全部原始结果、源码及二进制指纹；输出文件已经存在时拒绝覆盖。

**解析：** 相同字节数下，短记录增加调用、参数检查和外层循环次数。记录大小还可能改变优化器生成的循环，因此仅靠时间变化不能断言“全部差值就是函数调用开销”。调用计数和原始时间共同建立待检验的问题，不能跳到跨 Python/Lua 的通用结论。

## Part 2：受控对照

`batch` 将同一批字节交给一次调用；`copy_batch` 额外复制一次到已分配的缓冲区，再调用相同内核。它们都不包含 callback、插件状态计数或等待输入凑批，因为那些行为会改变比较契约。

```sh
python tools/run_costs.py --exe build/b01/Release/c18_boundary_cost.exe --variants per_record batch copy_batch --output build/costs/comparison.json
```

**解析：** 每轮调用数从 records 变成1是可检查的机制变化；额外复制字节数由驱动显式报告。时间差仍可能混合向量化、缓存和分支效果。只有为这些机制补充汇编/profile证据后，才能继续细分原因。对照是检验候选方向，课程没有把批量策略自动推广到有状态插件。

## Part 3：报告真实代价

程序固定状态重建、工作量和输入；初始化/分配在计时外，转换及 copy_batch 的额外复制在计时内，完整字节 oracle 在计时后。每次调用仍检查状态和写入长度。进程外超时覆盖整个执行；失败或源码/二进制在测量期间改变使报告失败。

**解析：** 不预设加速比；保持无收益、基线胜出和离散数据。单条尾延迟、GIL转换成本、Lua栈操作和动态装载不在本实验计时范围内；不要把 native kernel 计数当成已测这些边界。桌面其他负载未隔离，报告保留此限制。

本次运行产生的 JSON 留在 `build/`，不进入教材或Git。复现要求相同协议和可解释差异，不要求另一台机器取得相同耗时。
