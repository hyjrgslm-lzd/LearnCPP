# F-3 HALO 触发与失败诊断

对应文档：`08-模块F-协程帧与allocator.md` 「练习 F-3」。

## 目标

写两版 generator 消费代码——版本 A 帧地址从不逃逸（HALO 可触发），版本 B 故意把
帧地址写入全局变量（HALO 阻断）。用编译器 flag 诊断 HALO 是否触发，并实测两版的
每次迭代耗时差异。建立"HALO 不是黑盒魔法，而是条件明确的编译期优化"的直觉。

## 必做任务

1. 用 -O2 编译并运行：观察 `consumer_A` vs `consumer_B` 的耗时差异。
2. 用 HALO 诊断 flag 编译：
   - Clang：`-Rpass=coroutine-elide -Rpass-analysis=coroutine-elide`，期望 `consumer_A`
     输出 `coroutine frame elided` remark。
   - GCC：`-fdump-tree-coro`，检查 `simple_range` 是否仍有 `operator new`。
   - MSVC：Release build 反汇编，看帧分配点是否消失。
3. 在笔记中记录两版的耗时和编译器报告。
4. 列出至少 3 种破坏 HALO 的代码模式（除版本 B 之外）。

## 验收点

- 你能复述 HALO 触发的 3 个必要条件。
- 你能用编译器 flag 验证 HALO 是否生效。
- 你能用 ratio (B/A) 反推 HALO 是否被触发——预期 > 5x 表示触发，< 2x 表示没触发。
- 你能向同事讲清"HALO 是逃逸分析驱动的优化，任何帧地址逃逸路径都会让它静默失败"。

## 提示

- HALO 在 -O0 下几乎不会触发——一定要 -O1 以上。
- 如果你的 stdlib 还没有 `std::generator`，本题骨架内置了一个 demo::generator 仿制版。
- 性能实测时不要用 printf 计时——I/O 会把 HALO 的优化幅度淹没。
