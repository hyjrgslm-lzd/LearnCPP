# P1：批次原子性与日志恢复

先阅读 [第 14 章](../../chapters/14-log-and-checkpoint.md)。本单元由一个独立实现 Part 和四个观察/修改 Part 组成；实现题与完整存储驱动的通过含义不同。

| Part | 已提供与学生工作 | 编辑位置/检查 |
|---|---|---|
| 1 原子批次 | 已提供 mutation、dictionary、容量常量及检查器；自行完成校验、顺序应用和失败不发布 | student/solution.hpp；C12_P1_apply_student |
| 2 文件边界 | 完整 log_store/file 驱动；先预测每个故障点，再运行真实进程终止 | main.cpp、recovery.py；C12_P1_store/recovery |
| 3 恢复后再写 | 预测 A/B/C 两种确认历史，解释为什么修复尾部必须先于继续写入 | recovery.py 的实际输出和记录；说明 LSN 的来源 |
| 4 检查点中断 | 比较 half_snapshot 与 snapshot_flushed，指出回退使用哪个状态 | 日志、快照和实际恢复序号；不可把进程终止称作掉电 |
| 5 边界扩展 | 在自有练习副本补充一个不同键长/值长的截断用例及一个容量拒绝用例 | 检查调用实际存储 API，并提供失败前后状态的独立预期 |

Reference 位于 reference/solution.hpp，good 是独立完成体，bad 在部分修改后才发现非法输入。学生只编辑 student/solution.hpp；复制调用 Reference、修改检查器或预填输出均不构成实现。

```powershell
cmake -S C12_Serialization_Databases_Storage/exercises/P1_log_store -B build/c12-p1-student -DC12_BUILD_REFERENCE=OFF -DC12_TEST_STUDENTS=ON
cmake --build build/c12-p1-student --config Release
ctest --test-dir build/c12-p1-student -C Release --output-on-failure
```

初始状态输出 UNFINISHED 且失败。实现完成后的独立检查覆盖原子拒绝、重复键顺序、删除、值上限、空批和键数容量。用 `ctest --test-dir build/c12-p1 -C Release -V -R C12_P1_recovery` 查看每个案例的真实 JSON 输出；SNAP 是选中快照的序号，BAD 是拒绝的快照数，REPAIRED 是本次丢弃的不完整尾字节数。Part 2—5 的预测、解释和扩展需要人工核对，观察程序运行通过不等于这些作业已经完成。

解析：Part 1 先构建候选状态，只有整批成功才替换原状态；Part 2 的 after_flush 表示同步调用已成功但不表示响应已被父进程收到；Part 3 必须将完整未确认批计入历史；Part 4 只选择完整且历史匹配的快照；Part 5 的反例应证明检查器会拒绝错误，不能只给一个“最终数值正确”的正例。更完整的因果推导和误区解析见第 14 章第 8 节。


## Part 5 完整参考与判定

[extensions.py](extensions.py)先通过真实 store 写 A，再写键长更大的 B，保存真实日志。随后在父进程自有目录构造 B 的不同持久前缀，交给真实 open/get 恢复；未完整 B 的两个键都应 absent，完整 B 的两个键都应存在。再提交更短的 C、关闭并重新查询 A/C。这个输入能揭露“覆盖了一段坏尾就以为修好了”的实现：长尾剩余字节可能再次破坏解析。

[boundaries.cpp](boundaries.cpp)反复写有界合法大批次，直到真实 64 MiB 日志上限拒绝新批；拒绝必须是 capacity，已经确认的序号和值保持不变，并能重新打开。它另外按分配预算使准备/恢复发生 bad_alloc，检查 allocation 错误与重试后的原历史。

```powershell
ctest --test-dir build/c12-p1 -C Release -V -R "C12_P1_(extensions|boundaries)$"
```

这些是完整的参考扩展。学生应先自行选定一个不同长度、截断点和判定，再与参考比较；运行参考不会自动完成学生的扩展作业。两项测试均由外部监督器限制时间，并只使用其拥有的临时数据。
