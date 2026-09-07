# F-3 HALO 触发与失败诊断

对应正文：[08 模块 F](../../08-模块F-协程帧与allocator.md#f3)。

本练习诊断 coroutine state 的 heap allocation elision。重点是用编译器证据说明分配有没有被省略。

## Part 1：两个版本先跑通

版本 A 局部创建并消费 generator，返回对象和 handle 不逃逸。版本 B 故意把对象地址写入全局，给优化器制造逃逸路径。

先运行程序确认两版计算结果一致。性能计时中不要打印每个元素，I/O 会淹没分配差异。

## Part 2：用工具链证据判断

可选诊断：

```powershell
clang++ -std=c++23 -O2 -Rpass=coroutine-elide -Rpass-analysis=coroutine-elide main.cpp
g++ -std=c++23 -O2 -fdump-tree-coro main.cpp
```

MSVC 可在 Release 反汇编里找分配调用。记录编译器版本、优化级别、是否看到 `operator new` 或 elide remark。

## Part 3：分清三种结论

- “程序更快”只说明可能发生优化。
- “remark 显示 frame elided”说明当前 Clang 在此处做了 HALO。
- “汇编中没有分配调用”说明当前生成代码省掉了 heap allocation。

标准只允许实现省略分配，不保证任何具体代码形状一定触发。

## 验收

- 记录 A/B 两版耗时。

  **答案解析：** A/B 耗时只能作为线索：局部创建并消费的版本若明显更快，可能更利于 HALO 或内联；逃逸版本通常让优化器更难证明生命周期嵌套。时间受机器、优化级别、计时粒度和 I/O 影响，所以只记录本机结果和条件，不把耗时当最终证明。
- 记录至少一种编译器诊断证据。

  **答案解析：** HALO 的可靠证据要来自编译器输出，例如 Clang `coroutine-elide` remark、GCC coro dump、MSVC Release 反汇编，或分配调用是否消失。F-3 的 `hook_task` 还能通过 allocation hook 观察逃逸路径是否确实分配。诊断证据要写清编译器版本和优化级别。
- 列出三种破坏 HALO 的模式：返回对象逃逸、handle 逃逸、跨编译单元不透明消费等。

  **答案解析：** HALO 依赖实现能证明 coroutine state 生命周期严格嵌套在调用方内，且大小在调用点可知。返回对象逃逸、保存裸 `coroutine_handle`、跨编译单元/虚调用隐藏消费路径，都会让这个证明变困难或失败。F-3 的版本 B 用逃逸路径制造对照。
- 解释 HALO 处理 frame allocation，E-3 ready 短路处理 await-expression 控制流。

  **答案解析：** HALO 只讨论整个 coroutine state 的 heap allocation 是否省略；触发后语义仍像存在 frame 一样。ready 短路讨论单个 `co_await` 的控制流，`await_ready()==true` 时跳过 `await_suspend()`。一个协程可以有 ready 短路但仍分配 frame，也可以有真实挂起点但在某个实现上被 elide。

## Reference

Reference 是稳定可编译 fixture；通过 compiler remark/dump/disassembly 判断是否 elide heap allocation，不能用微基准时间当作唯一证明。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target F3_halo_diagnose F3_halo_diagnose_reference
ctest --test-dir build/dg-lane -C Release -R F3_halo_diagnose_reference
```
