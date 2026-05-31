# 练习 J-2：跨编译器 ABI 与符号问题

对应文档：`12-模块J-陷阱诊断与跨编译器.md` §J-2

## 目标

同一份最小 `task<T>` 实现，分别在 MSVC / Clang / GCC 上编译。对比：

- 协程帧大小（empty body vs 5 个 int 局部）；
- HALO 触发情况（同步路径 vs 含 co_await 的路径）；
- 调试信息里 frame 变量的可读性；
- `coroutine_handle<>` 的 `sizeof`；
- 跨 DLL 传递 `coroutine_handle<>` 的崩溃模式。

最终建立明确认知：**协程 ABI 不在 C++ 标准内**，跨模块/跨编译器传递
`coroutine_handle` 是危险操作。

## 必做任务

1. 用 `task<int>` 准备 4 个 demo：`empty_body` / `five_ints` / `with_await` /
   跨 DLL 工厂；
2. 在三种编译器上分别用以下 flag 编译并采集数据（CMakeLists 已自动注入）：

   | 编译器 | 帧布局 dump | HALO 诊断 |
   | --- | --- | --- |
   | MSVC 17.10+ | `/d1reportSingleClassLayoutpromise_type` | （无公开 flag，看汇编里 `operator new`） |
   | Clang 17+ | `-Xclang -fdump-record-layouts` | `-Rpass=coroutine-elide` |
   | GCC 14+ | `-fdump-tree-coro` | `-fdump-ipa-coro` |

3. 填完下表（自己跑数据）：

   | 指标                          | MSVC | Clang | GCC |
   | ----------------------------- | ---- | ----- | --- |
   | 协程帧大小（empty_body）       |      |       |     |
   | 协程帧大小（five_ints）        |      |       |     |
   | HALO 触发（empty_body）        |      |       |     |
   | HALO 触发（with_await）        |      |       |     |
   | 调试信息中 frame 变量可读性     |      |       |     |
   | sizeof(coroutine_handle<>)    |      |       |     |

4. 跨 DLL 实验：把 `make_task_for_dll_demo()` 拆到一个 DLL，
   把 `main()` 留在 EXE。当 DLL/EXE 编译选项不同（Debug DLL + Release EXE，
   或不同编译器）时，观察是否成功 / 崩溃 / 堆损坏。

## 验收点

- 至少填完两份对比表（帧大小 + HALO）；
- 跨 DLL 实验亲手跑过，能解释崩溃（或不崩溃但不可靠）的根因；
- 能用一句话说清"为什么协程 ABI 不在标准内"——帧布局/异常处理实现是编译器
  私有选择，标准化它们等于锁死优化空间；
- 能为团队制定一条规则：**禁止在 DLL 边界暴露 `coroutine_handle<>`** 或
  依赖跨编译器协程帧布局兼容性。

## 约束

- 三个编译器使用相同优化级别（`-O2` / `/O2`）和相同 C++ 标准（C++23）；
- 跨 DLL 实验用最小 task（`co_return 42`），减少干扰因素；
- 不依赖 HALO 作为性能假设——它是优化而非保证。

## 提示

- 先在一个编译器上跑通全部实验，再扩展到另两个；
- MSVC 调试信息对协程支持最好（Parallel Stacks 原生展示协程链）；
- GDB 14+ `info coroutines` 是 Linux 端的协程调试基础设施。
