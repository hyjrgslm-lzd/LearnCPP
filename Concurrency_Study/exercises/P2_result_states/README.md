# P2：在单线程里观察结果状态

先读 [02 结果通道](../../chapters/02-result-channels.md)。[main.cpp](main.cpp) 提供能结束的设值/取值基线，[solution.cpp](solution.cpp) 完整实现四个必做 Part。暂时不要创建线程；本题要证明 future 并不要求后台执行。

## Part 与答案

### Part 1：有效不等于就绪

增加默认 future，移动一个有效 future，再依次查询、设值、重复 wait、get。对应 `part1_states()`。默认句柄 valid=false；get_future 后 valid=true 但查询 timeout；move 后源无效、目的有效；设值后 ready；wait 保持有效；get=42 后无效。

不要在未设值时在唯一生产线程阻塞 get。不要在 get 后再 get 或 wait；对无效 future 的这些调用不是可移植的异常测试。

### Part 2：异常也是 ready

在 catch 中 set_exception，然后先 wait 再 get。对应 `part2_exception()`。wait 返回但不重抛存储异常；get 重抛消息为 calculation failed 的 runtime_error，并使句柄失效。Reference 同时检查异常类型、消息及失效状态，漏传异常不能误判为成功。

### Part 3：一次完成与放弃

对应 `part3_provider_errors()`。重复 get_future 抛 future_already_retrieved；重复 set_value 抛 promise_already_satisfied，第一次的结果仍为 1；未满足 promise 被销毁后，结果状态 ready，get 抛 broken_promise。

这三个是有规定语义的可运行错误情形，和越界、悬垂或无效 future.get 不同。broken_promise 表示完成责任被放弃，不代表运行时保留了某个未发送的业务异常。

### Part 4：同一结果与无值事件

对应 `part4_shared_and_void()`。share 使普通 future 无效；两个 shared_future 副本关联同一字符串，get 返回 const string& 并可重复读取。Reference 检查地址、内容和返回类型；需要独立寿命时显式复制字符串。随后用 promise<void> 发一个完成事件，get 不返回值但仍消费句柄。

## 验收与边界

输出四段状态轨迹与 `P2_reference OK`。单线程事件顺序来自代码，不靠时间戳。此题尚未证明跨线程访问或 async 析构行为，分别由 D1/D2 检查。

规范：[future](https://eel.is/c++draft/futures.unique.future)、[共享状态](https://eel.is/c++draft/futures.state)、[shared_future](https://eel.is/c++draft/futures.shared.future)。固定基准为正文链接的 N5050，滚动页面不能代替版本核对。

## 构建与运行

从 `Concurrency_Study/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S P2_result_states -B build/P2_result_states -G "Visual Studio 18 2026" -A x64
cmake --build build/P2_result_states --config Release
./build/P2_result_states/Release/P2_result_states.exe
ctest --test-dir build/P2_result_states -C Release --output-on-failure
```

统一构建注册的检查目标是 `P2_result_states_reference`，CTest 使用进程级超时。新增叶项目 CMake 由课程主线程集成；尚未接线时，可在 VS x64 Native Tools 环境从本题目录直接验证（输出也留在本题目录）：

```powershell
cl /nologo /std:c++23preview /EHsc /utf-8 /W4 /O2 /DNDEBUG /I../include solution.cpp /FoP_reference.obj /FeP_reference.exe
./P_reference.exe
```

直接运行没有 CTest 的进程超时；本题是有限单线程检查。
