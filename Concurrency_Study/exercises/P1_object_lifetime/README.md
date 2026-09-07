# P1：用对象日志建立执行先修

完整正文见 [00 执行与对象](../../chapters/00-execution-and-objects.md)。本题只有单线程，不需要先学习锁或 future。[main.cpp](main.cpp) 是安全 Starter，[solution.cpp](solution.cpp) 是六个 Part 的完整 Reference。默认 C++23。

## Part 与答案

### Part 1：一个名字背后是哪个对象

在内层作用域构造两个有编号的对象，预测退出时的事件顺序，再运行 `part1_lifetime()`。答案为 `1,2,-2,-1`：完整构造的局部对象依逆序析构。外部 trace 比观察对象活得更久，因此析构可以安全记录它。

Reference 的记录器只为本题两个对象准备四项固定存储，不是通用日志容器。不要增加第五次事件却仍使用该数组；要扩展实验应同步扩大容量与检查。

### Part 2：副本、借用与所有权移动

Starter 先展示 snapshot=10、borrowed=20。把值捕获改为 mutable 递增，再加入独占指针的初始化捕获。Reference 检查副本为 11、外部变量先为 20，再由引用闭包改为 21；unique_ptr 移动后原指针为空，新的闭包持有值 42。

继续移动闭包后只调用新对象。答案不是“std::move 销毁了原对象”，而是该类型的移动转移资源，原闭包仍存在但已不持有可解引用的指针。闭包不可复制由 static_assert 检查。

### Part 3：std::invoke 与 std::ref

分别修改 counter 的副本与原对象，用 invoke 调用成员函数和访问数据成员。Reference 检查副本为 3，原对象为 7，普通 lambda 调用得到 42。invoke 处理调用规则；ref 保留借用语义；两者都不会新建线程或延长原对象寿命。

### Part 4：异常展开中的 RAII

在两个完整构造的对象之后抛出异常，在外层 catch 后核对记录。答案仍为 `1,2,-2,-1`，handler 看到资源已清理。不要从析构函数抛异常或调用会分配的观察逻辑，把新的失败混进要观察的现象。

### Part 5：用 tuple 保存参数，先区分拥有与借用

对应 `part5_tuple_ownership()`。将同一组 number=10、text="saved" 分别放进 make_tuple、tie，以及 `make_tuple(ref(number), cref(text))`。用静态检查确认三者类型分别为 `tuple<int,string>`、`tuple<int&,string&>`、`tuple<int&,const string&>`。

逐项答案：修改原变量为 20/"later" 后，拥有元组仍为 10/"saved"，借用元组读取 20/"later"；make_tuple 对 reference_wrapper 有解包规则，元素地址就是原变量地址。显式保存 `tuple<reference_wrapper<int>>` 时，元素是包装器，通过 `.get()` 写 22 后三个借用路径均观察到 22。再向 tie 元组赋入 30/"assigned"，原变量被更新，拥有元组不变。这些结果均由 Release 下仍执行的 cs::check 核验。

不能把“拥有 tuple 对象”当成“拥有它引用的所有对象”。移动 tie 元组后取出的引用元素仍是 int&；保存指针、视图或引用包装器，都不替目标延寿。`forward_as_tuple` 也不会延长临时对象寿命，本题不把它用于保存稍后执行的临时参数。

### Part 6：保存 callable 和参数包，稍后 apply

对应 `part6_saved_arguments()`。先把 callable 和 make_tuple/tie 分别捕获进闭包，再修改原变量，最后才调用。逐项答案：保存后的调用次数为 0；值参数保留 10+3，结果 13；借用参数观察 20+4，结果 24；两次调用后次数为 2。apply 将参数包展开并在当前线程调用，没有排队或后台执行。

独占参数版本将 unique_ptr(37) 和增量 5 保存进 tuple，再将 tuple 移入闭包，在调用时 `apply(fn, move(args))`，由按值形参取得指针。Reference 检查原指针、移出后的外部 tuple、调用后的闭包 tuple 中的指针依次为空，结果为 42；静态检查确认左值 unique_ptr 参数不能复制给按值形参，右值可以。普通闭包不会自动管理“一次性”状态，本例消费输入后不再调用。

## 验收与边界

运行 Reference 应显示六个 Part 的解释和 `P1_reference OK`。cs::check 在 Release 仍执行。Starter 只验证初始捕获与值参数快照，不能替代六个 Part。故意悬垂引用和解引用已移出的资源只在正文讨论，不参与默认运行。

规范：[对象寿命](https://eel.is/c++draft/basic.life)、[lambda 捕获](https://eel.is/c++draft/expr.prim.lambda.capture)、[invoke](https://eel.is/c++draft/func.invoke)、[tuple 创建](https://eel.is/c++draft/tuple.creation)、[apply](https://eel.is/c++draft/tuple.apply)。固定版条款号按正文 N5050 说明核对，eel 为滚动导航。

## 构建与运行

从 `Concurrency_Study/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S P1_object_lifetime -B build/P1_object_lifetime -G "Visual Studio 18 2026" -A x64
cmake --build build/P1_object_lifetime --config Release
./build/P1_object_lifetime/Release/P1_object_lifetime.exe
ctest --test-dir build/P1_object_lifetime -C Release --output-on-failure
```

统一构建注册的检查目标是 `P1_object_lifetime_reference`，CTest 使用进程级超时。新增叶项目 CMake 由课程主线程集成；尚未接线时，可在 VS x64 Native Tools 环境从本题目录直接验证（输出也留在本题目录）：

```powershell
cl /nologo /std:c++23preview /EHsc /utf-8 /W4 /O2 /DNDEBUG /I../include solution.cpp /FoP_reference.obj /FeP_reference.exe
./P_reference.exe
```

直接运行没有 CTest 的进程超时；本题是有限单线程检查。
