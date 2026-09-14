# 05. Callback drain and plugin shutdown

这一章只解决一个小但真实的问题：host 正在调用插件，插件又同步回调 host 请求停止；与此同时，另一个线程可能开始 `close()`。此时谁能持锁，谁负责等待，什么时候可以 `destroy`，什么时候可以释放本次动态库 handle。

主案例仍是二进制记录处理。插件把 ASCII `a-z` 转成 `A-Z`；NUL、`0xff`、标点和已经大写的字节保持原样。输出长度总是等于输入长度。容量不足时返回 `C18_STATUS_BUFFER_TOO_SMALL`，`written` 写入需要的长度，并且不修改业务输出和 session 状态。空输入合法。

## ABI contract

公共 C ABI 在 `exercises/include/c18/abi.h`。动态库只导出一个入口：

```c
c18_status c18_get_api(uint32_t requested_version,
                       uint32_t host_struct_size,
                       c18_api* out_api);
```

`c18_api` 是 C11 函数表，字段只有 `create/process/request_stop/destroy`。host 传入版本号和自己能接收的表大小；插件返回同版本、足够大小、函数指针完整的表。`c18_status` 固定为 `uint32_t`，避免 `enum` 宽度被编译选项改变。`size_t` 只适合同进程、同架构 native ABI；Python `ctypes` 侧要用 `c_size_t`，不能把它当跨 32/64 位架构稳定线格式。长期对象只以 `c18_context*` opaque handle 暴露。这里没有 `std::string`、`std::vector`、异常、跨 CRT 分配，也没有让调用方释放插件分配的内存。

函数表的责任很窄：

- `create` 接收 `c18_host_api`，保存 host callback 和 userdata；失败时 `out` 必须清为 `nullptr`。
- `process` 接收显式指针、长度、输出容量和 `written`。
- `request_stop` 只请求进入关闭，不等待。
- `destroy` 只在 host 已经 drain 完 active call/callback/borrow 后调用。

## Part 1: serial baseline

最朴素的正确插件完全串行：

1. `create` 先把 `*out = nullptr`，再检查 host 表版本和大小，最后分配 `c18_context`。
2. `process` 先写 `*written = input_size`。
3. 如果 closing，返回 `C18_STATUS_CLOSING`。
4. 如果容量不足，返回 `C18_STATUS_BUFFER_TOO_SMALL`。
5. 逐字节转换到 output。
6. `request_stop` 设置 closing。
7. `destroy` 删除 context。

这个基线已经覆盖很多 ABI 雷区：长度不用 NUL 结尾推断；small buffer 不写半截结果；异常在插件内部翻译成状态码；C ABI 之外的对象不暴露。它还没有覆盖关闭并发。

## Part 2: reproduced gap

L04 fixture 在看到字节 `!` 时同步调用 host callback，事件名是 `process:bang`。host callback 做两种事：

- 串行检查中，它调用 `session.request_stop()`，证明 callback 内只请求停止、不等待自己 drain。
- 并发检查中，它停在 condition variable 栅栏上；主线程先调用 `request_stop()`，再启动 `close()`。

这个轨迹不是把同步 callback 冒充并发。真实并发点是：一个线程仍在 `process -> plugin -> host callback` 栈上，另一个线程已经调用 `request_stop/close`。checker 等到因果事件出现：callback 已进入、plugin 收到 stop、`close` 到达 drain wait，或者坏实现提前返回。timeout 只作外层安全网；timeout 或 crash 都是失败，不算拒绝成功。

旧坏实现只测“host 持锁进入 `process`”。它能发现死锁类错误，但漏掉更危险的半关闭：`close()` 在 callback 还没返回时就 `destroy` 或释放 loader handle。新 checker 用 `process` + callback 栅栏复现这个缺口；bad 必须稳定输出：

```text
check failed: close returned before callback drained
```

## Part 3: root cause

根因不是“少一个锁”。根因是把 session 所有权看成单个 bool，而不是跨 ABI 调用期间的 lease。

所有插件 callout 都必须遵守同一形状：

1. 短暂持 session 锁。
2. 检查状态，复制函数指针和 `c18_context*`。
3. 增加 `active_calls`。
4. 释放 session 锁。
5. 调用插件。
6. 重新持锁递减 `active_calls`，通知 drain。

`process`、`request_stop`、`destroy` 都是插件 callout。`c18_get_api/create` 也不能在持 host 锁时调用；它们在 `open` 的本地变量中完成，成功后再短锁发布 api、ctx 和 loader handle。

callback trampoline 同样短锁：增加 `callbacks`，复制用户 callback，释放锁，调用用户 callback，返回后递减并通知。这样 callback 内可以调用 `request_stop()`，但不能调用等待自己结束的 `close()`。

## Part 4: state and lease

本题使用的状态是：

```text
empty -> running -> closing -> destroying -> context_destroyed -> empty
```

课程计划里的 `handle_released` 在这个类中折回 `empty`：context 已 destroy 且 loader handle release 成功后，session 可以再次 `open`。关键不是状态名，而是两个阶段不能混在一起：

- `destroy` 失败：保留 `ctx`、函数表和 loader handle，状态回到 `closing`，下一次 `close()` 可以重试 destroy。
- `destroy` 成功但 loader release 失败：清掉 `ctx`，保留函数表和 loader handle，下一次 `close()` 只重试 release，不能再次 destroy 旧 `ctx`。

checker 用两个证据覆盖这点：`busy_destroy` 插件第一次 `destroy` 返回 `C18_STATUS_BUSY`，第二次才成功；release 失败由 session 的测试钩子模拟，第一次 `close()` 返回 busy，第二次只 release handle，`destroy:called` 计数仍为 1。

## Part 5: fixed implementation

参考答案把 callback userdata 放在一个小 `Control` block 里。这个 block 有真实多所有者理由：插件只知道 C 指针；host 对象要在 close 失败时避免留下 dangling userdata。正常路径下 `PluginSession` 拥有它；析构时如果 `close()` 失败，答案宁可泄漏 retained control、ctx 和 loader handle，也不释放仍可能被 callback 使用的状态。

这不是支持“对象析构和成员函数并发”。合法用法前提仍然是：外部 owner 必须保证没有线程还在调用这个 `PluginSession` 对象的成员，再让对象析构。`close()` 可以和已经进入插件的 callout 并发；对象本身的生命周期必须由外部更长的 owner 覆盖。

实现代价很小：一个 mutex、一个 condition variable、`active_calls/callbacks/borrows` 三个计数。L04 没有公开 borrow API，只保留计数位置，后续 Python/Lua 章节会把外部语言对象 lease 接上。

## Exercise

补全 `exercises/L04_plugin_shutdown/student/solution.hpp`：

1. 用 Windows `LoadLibraryExW` 或 Linux `dlopen(RTLD_NOW | RTLD_LOCAL)` 加载绝对路径动态库。
2. 找到唯一入口 `c18_get_api`，协商 `C18_ABI_VERSION` 和 `sizeof(c18_api)`。
3. 实现 `process` 的状态检查、active call 计数和异常到状态码的翻译。
4. 实现 host callback trampoline，保证调用用户 callback 时不持 session 锁。
5. 实现 `request_stop` 和 `close`，按 closing -> drain -> destroy -> release 顺序收束。
6. 处理失败重试：destroy 失败保留 context；release 失败保留 loader handle。

命令：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L04_plugin_shutdown -B C18_Interoperability_Plugins_Tooling/build/sample-author-l04 -DC18_BUILD_REFERENCE=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/sample-author-l04 --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/sample-author-l04 -C Release -E student --output-on-failure
```

`C18_TEST_STUDENTS=ON` 会注册未完成 Student；它应该编译，但运行失败，证明 checker 没有调用 Reference 替学生答题。

## Reference reading guide

参考答案在 `exercises/L04_plugin_shutdown/reference/solution.hpp`。按四段读：

- `open`：动态加载、查符号、版本/表大小/函数指针检查、`create`，全程不持 host 锁调用插件。
- `process/request_stop`：短锁锁定状态、函数、ctx 和计数；锁外 callout；退出通知 drain。
- `host_event`：短锁增加 callback lease，锁外执行用户 callback，返回后递减。
- `close`：进入 closing，等待 active call/callback/borrow 归零，destroy；destroy 成功后再 release handle；任一阶段失败都保留可重试所有权。

`validation/good/solution.hpp` 是独立好实现。它不 include Reference。`validation/bad/solution.hpp` 是真实坏实现：能加载、能处理、能 request stop，但 `close()` 不等待 callback drain。`fixtures/good_plugin.cpp` 是真实共享库，也被 P1 复用；`missing_symbol.cpp`、`version_mismatch.cpp`、`throwing_entry.cpp` 分别覆盖缺入口、版本拒绝和入口错误翻译。

本章不承诺强制物理卸载。操作系统 loader 可能因为别的引用保留映像，插件也可能恶意保存 host 指针。这里能承诺的是：本 session 不再调用已释放 handle 的函数表；已知 call/callback/borrow drain 前不会 destroy；drain、destroy 或 release 失败时不会释放一半所有权。
