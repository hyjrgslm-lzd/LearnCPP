# F-1 观察协程帧布局

对应文档：`08-模块F-协程帧与allocator.md` 「练习 F-1」。

## 目标

用编译器 flag 打印协程帧（coroutine frame）的内存布局，识别帧内的四大区域——
promise、参数副本、resume_index、局部变量 spill 区——把"协程帧"从抽象概念变成
可观测的 struct。

## 必做任务

1. 编译 `main.cpp`（任选一个编译器）：
   - GCC：`g++ -std=c++23 -fdump-tree-coro main.cpp`，查看 `.c.022t.coro` 等 dump 文件。
   - MSVC：`cl /std:c++latest /d1reportSingleClassLayoutobserver_task::promise_type main.cpp`。
   - Clang：`clang++ -std=c++23 -Xclang -ast-dump -fsyntax-only main.cpp`。
2. 在 dump 输出中找到 `observed` 协程的 frame 结构体定义。
3. 在帧中标注 promise / `param_a/b/c` 副本 / resume_index / `local_x/local_y/local_str` 的相对位置。
4. 比较 `observed`（多变量多挂起）、`observed_complex`（含 vector/unique_ptr）、
   `observed_minimal`（极简版）三者的帧大小差异。
5. 在笔记里画一张 ASCII 帧布局图。

## 验收点

- 你能在 dump 中定位到协程帧 struct 的真实字段顺序。
- 你能解释：为什么参数 `std::string` 的副本必须存在于帧中而非栈上？
- 你能解释：为什么 `resume_index` 必须是帧内字段，而不能由 IP 推算？
- 你跑通了至少一个编译器的 dump 流程并完成标注。

## 提示

- GCC `.coro` dump 文件名形如 `main.cpp.022t.coro`，需到 build 目录下找。
- 极简协程的帧 dump 通常更易读——先用 `observed_minimal` 练手再去看 `observed`。
- Godbolt 上可以即时跑这些 flag，省去本地配环境。
