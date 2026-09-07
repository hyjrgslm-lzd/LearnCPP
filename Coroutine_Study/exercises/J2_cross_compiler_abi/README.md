# 练习 J-2：本机协程对象与 ABI 观察

先阅读 [模块 J 的对象布局与 ABI 讲解](../../12-模块J-陷阱诊断与跨编译器.md#j2)。本题从同一翻译单元里的可观察结果开始，再讨论模块边界需要保存哪些约定。

## Part 1：运行当前实现

`main.cpp` 提供观察骨架，`solution.cpp` 提供四个完整路径：

- `empty_body()` 返回 0。
  **答案解析：** 这条路径只验证最小 task 的创建、启动、`co_return`、`final_suspend` 和 `destroy` 能闭环。它不能说明 frame 布局大小，只能作为后续 ABI 观察的基线。
- `five_ints()` 把五个局部整数求和，返回 15。
  **答案解析：** 五个局部变量会影响当前编译器对协程状态和 spill 的安排，但标准不规定这些字段如何布局。返回 15 只证明局部状态跨协程执行过程保持正确，具体存储位置要结合分配 size、dump 或优化诊断看。
- `with_await()` 使用两个立即就绪的 `ready_int`，返回 42。
  **答案解析：** `ready_int.await_ready()` 为 true，所以这两个 await 不进入挂起分支。它适合观察编译器是否消除部分协程开销，但实际分配次数仍依赖工具链、优化级别和调用形状。
- `abi_boundary_shape()` 返回 42，用来讨论导出接口的形状。
  **答案解析：** 这条路径提醒模块边界应暴露普通结果、回调、future 或 opaque token。裸 `coroutine_handle` 的 resume/destroy 函数、promise 偏移和分配释放策略都属于同一实现侧细节，跨 DLL/so 暴露会把工具链 ABI 变成接口承诺。

Reference 中的这几条路径都能同步完成，所以它的局部 `sync_wait()` 启动后即可消费结果。外部事件驱动的等待与完成通知在 G3 中展开。

从 `Coroutine_Study/exercises` 编译：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --target J2_cross_compiler_abi J2_cross_compiler_abi_reference
```

Visual Studio 默认构建目录中的运行入口：

```powershell
./build/verify-core/J2_cross_compiler_abi/Release/J2_cross_compiler_abi_reference.exe
```

## Part 2：解释每个观测值

记录本机编译器、标准库、构建配置和程序输出：

| 观测项 | 它实际说明什么 | 本机记录 |
| --- | --- | --- |
| `sizeof(task<int>)` | 返回包装对象的体积 | |
| **答案解析** | 本例 `task<int>` 只保存一个 `std::coroutine_handle<promise_type>`，所以它的大小通常等于一个指针大小；实际值以本机输出为准。这个数值描述返回对象，不描述完整 coroutine frame。 | |
| `sizeof(coroutine_handle<>)` | 句柄对象的体积 | |
| **答案解析** | `coroutine_handle<>` 是标准化的非拥有控制句柄，常见实现也是一个指针大小；标准保证可用操作，不保证跨实现 ABI 布局。判断时记录编译器、标准库、位数和优化配置。 | |
| `is_trivially_copyable_v<coroutine_handle<>>` | 句柄复制相关的类型性质 | |
| **答案解析** | trivially copyable 只说明句柄对象本身可以按普通对象复制，不说明复制后拥有 frame。所有复制出的 handle 都指向同一 coroutine state，谁能 `destroy()` 仍由 task/owner 契约决定。 | |
| `promise allocations observed` | 四条路径合计调用 promise 分配函数的次数 | |
| **答案解析** | 计数来自本例 `promise_type::operator new`，当前 reference 四条路径各创建一次 task，未被 HALO 消除时常见观察是 4。优化器若证明生命周期严格嵌套，可能减少或消除分配；因此报告时写实际输出和构建选项，不写成跨平台固定值。 | |

task 包装对象在本例中保存一个 handle；协程帧保存 promise、执行状态等另一组数据。因此，要研究完整帧的存储需求，应结合 F 模块中分配函数收到的 size、对象日志和编译器产物。这里先把“返回对象体积”和“被它管理的状态”联系起来。

分配次数受编译器、优化选项和具体调用形状影响。记录实际数值后，沿 `promise_type::operator new` 看计数来自哪里，再检查是否有分配消除的证据。

## 当前构建选项

本目录 CMake 为当前使用的编译器设置相应选项：

| 编译器 | 选项 |
| --- | --- |
| MSVC | `/Zc:__cplusplus /utf-8 /await:strict /Zi` |
| Clang | `-Rpass=coroutine-elide -Rpass-missed=coroutine-elide` |
| GCC | `-fdump-tree-coro` |

分析时先阅读本机实际产生的诊断和构建命令。不同工具链的布局输出格式随实现变化，具体使用方式应对照所用版本的编译器文档。

## Part 3：把观察用于接口设计

语言规定 handle 的可用操作和前置条件；具体协程状态布局由实现决定。一个模块如果向外暴露恢复和销毁能力，就需要同时约定工具链、运行时、分配释放和对象存活关系。

给本例画一张接口图：创建 task 的模块、读取结果的调用方、执行 resume/destroy 的位置分别在哪里。尝试把模块边界设计成结果值、回调或由提供方负责释放的 opaque token，说明各自怎样保留所有权。

**答案解析：** 图里应把创建 task 的实现侧画成 frame owner，把读取结果的调用方画成 value/error 消费者，把 `resume/destroy` 留在同一工具链/运行时边界内。结果值接口通过拷贝/移动值切断 frame ABI；回调接口由提供方在完成时调用用户函数；opaque token 接口让提供方继续管理释放，调用方只持有受控句柄。

当学习环境具备另一套工具链或独立动态库时，可以扩展记录不同优化配置和 DLL/so 的表现。先写清两个模块的创建、访问、销毁契约，再研究实现差异。

完成本题时，应能根据实际输出解释包装对象、handle、协程状态和分配次数之间的关系。

**答案解析：** 包装对象是外部返回的 owner/入口，本例主要保存 handle；handle 是非拥有控制杆，指向 coroutine state；coroutine state 包含 promise、执行状态、跨挂起局部对象和可能的异常状态。分配次数只反映当前编译器在当前配置下是否调用了 promise `operator new`，不能反推出标准 ABI 或固定 frame 布局。
