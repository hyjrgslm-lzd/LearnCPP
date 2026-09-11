# F-1 观察协程帧布局

对应正文：[08 模块 F](../../08-模块F-协程帧与allocator.md#f1)。

本练习要把“协程帧”从抽象名词变成当前编译器能观察到的状态对象。标准不规定字段顺序；你要记录的是自己工具链的实现证据。

## 做题前先判断哪些状态会留下来

`main.cpp` 里有三个协程：

- `observed`：参数、字符串局部、两个挂起点，适合看完整 frame。
- `observed_complex`：加入更复杂局部对象，适合看非平凡析构和 size 增长。
- `observed_minimal`：极简对照，适合先找到编译器生成的 frame 名字。

跨挂起点仍要使用的参数副本、局部对象、awaiter 临时和 resume point 都可能进入 coroutine state。只在挂起前使用的临时，优化后可能消失。

## Part 1：生成 dump

任选一个工具链：

```powershell
g++ -std=c++23 -fdump-tree-all main.cpp
cl /std:c++latest /d1reportSingleClassLayoutobserver_task::promise_type main.cpp
clang++ -std=c++23 -Xclang -ast-dump -fsyntax-only main.cpp
```

如果本地工具链不支持对应 flag，可在 Godbolt 上运行相同代码。记录编译器版本和优化级别。

## Part 2：标注 frame

在 dump 中找到 `observed` 对应的 coroutine frame，标注：

```text
promise
resume index / state
param_a / param_b / param_c
local_x / local_y / local_str
awaiter 临时对象
销毁路径簿记
```

字段名字可能是 mangled name 或编译器内部名。写报告时以“我观察到的 GCC/MSVC/Clang 输出”为准。

## Part 3：解释生命周期

Reference 用 RAII marker 验证：`initial_suspend` 前协程体局部尚未构造；第一次挂起后，跨挂起局部仍存活；协程完成或 destroy 时按作用域析构。

回答两个问题：

1. `std::string param_c` 按值传参时为什么要进入 frame。

   **答案解析：** 按值参数在 coroutine replacement body 开始处创建副本，协程体内的参数名指向这份副本。`param_c` 在挂起后仍可能被读取，所以副本必须随 coroutine state 存活到恢复点之后。调用方离开后，frame 里的字符串副本仍有效。
2. 若改成引用或 `string_view`，调用方对象提前销毁后为什么会悬空。

   **答案解析：** 引用参数副本保存的是对外部对象的绑定，`string_view` 保存的是指针和长度，它们都不拥有字符存储。协程挂起后调用方对象若先析构，frame 中只剩指向已失效对象或缓冲区的引用关系。恢复后访问它就是悬空访问。

## 验收

- 找到至少一个协程的 frame 或等价 IR/AST 状态结构。

  **答案解析：** 这条验收要的是当前工具链证据，例如 GCC coro dump、Clang AST/IR、MSVC 反汇编或 Godbolt 输出。字段名字可以是编译器私有名，关键是能对应到某个协程生成的 state。没有这类证据时，只能说理解了语义，不能说观察到了布局。
- 标注 promise、参数、resume state、跨挂起局部。

  **答案解析：** 这些是 frame 中最该找的功能区域：promise 存结果和协议状态，参数副本或引用支撑恢复后继续访问，resume state 记录下次从哪里执行，跨挂起局部保持对象寿命。F-1 的 `observed` 专门包含这些状态。
- 对比三个协程的 frame size 或可见字段差异。

  **答案解析：** `observed_minimal` 用来找最小框架，`observed` 增加参数、字符串和挂起点，`observed_complex` 增加非平凡对象。大小和字段差异来自编译器、优化级别、ABI 和代码形状，所以要写观察方法和来源，不写固定数值。若某字段被优化掉，也要按当前输出记录。
- 把实现观察和标准语义分开记录。

  **答案解析：** 标准规定 coroutine state 要支撑参数副本、promise、恢复和销毁语义，但不规定字段顺序、padding、名字和具体大小。dump/IR/汇编只能说明当前工具链如何实现。报告里用“当前 MSVC/GCC/Clang 输出显示”这类表述，避免把 QoI 写成语言规则。

## Reference

Reference 验证局部生命周期，不猜测编译器私有字段。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target F1_frame_layout F1_frame_layout_reference
ctest --test-dir build/dg-lane -C Release -R F1_frame_layout_reference
```
