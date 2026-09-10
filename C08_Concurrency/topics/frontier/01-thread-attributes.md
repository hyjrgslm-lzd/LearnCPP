# C++29 线程属性：创建前的名字和栈大小建议

[P2019R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p2019r9.pdf) 给 `std::thread` 和 `std::jthread` 增加线程属性参数。[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html) 的 LWG Poll 18 说明它已应用到 N5054 工作草案。目标很窄：在新线程开始执行前，把调试名字和栈大小建议传给实现。它不标准化亲和性，不提供创建后查询接口，也不保证平台会按字节精确满足栈大小。

## 1. 属性放在可调用对象之前

新构造形状是：

```cpp
std::thread t(
    std::thread::name_hint<char>{"worker"},
    std::thread::stack_size_hint{256 * 1024},
    [] { /* work */ });
```

构造参数从左到右扫描，开头连续的 thread attribute 是属性；第一个非属性参数是可调用对象；后面才是传给可调用对象的参数。若只给属性没有可调用对象，程序不成形。同一种属性类型不能重复出现。这个顺序让已有 `std::thread(f, args...)` 可以在前面插入属性，而不用改业务参数列表。

`jthread` 也提供对应别名：

```cpp
std::jthread t(
    std::jthread::name_hint<char>{"poller"},
    std::jthread::stack_size_hint{0},
    [](std::stop_token st) { while (!st.stop_requested()) break; });
```

`stack_size_hint{0}` 表示忽略这个属性。非零值是建议，标准允许实现按平台要求上调或下调。

## 2. name_hint 是借用，不是保存字符串

`thread::name_hint<char>` 保存的是 `basic_string_view<char>`；对象不可复制、不可移动，目的是接近具名参数，避免为了创建线程复制一份可能很长的名字。推荐实践是不把属性值存进 `thread` 或 `jthread` 对象。属性对象只需要活到构造调用完成，之后销毁不影响线程对象。

这带来一个实际边界：传入的字符序列必须在构造函数读取它期间有效。字面量和调用前已经存在的 `std::string` 通常没问题；`std::thread(std::thread::name_hint<char>{std::string("worker")}, []{})` 这类完整表达式内的临时 string 会活到构造调用结束，因此属性对象在构造期间可读。错误写法是把 hint 或 view 跨语句保存：

```cpp
auto make_hint() {
    std::string s = "worker";
    return std::thread::name_hint<char>{s}; // 返回后 hint 里的 view 悬垂
}
std::thread t(make_hint(), [] {});
```

P2019R9 推荐实现不把属性存在 `thread` 对象里；它也没有要求属性对象拥有字符串。因此跨语句悬垂是调用方错误，不能靠线程对象后续持有名字来补救。

名字按 literal encoding 解释。P2019R9 本轮只接受 `char`，没有把 `char8_t`、`wchar_t` 或平台宽字符接口纳入同一标准属性。

## 3. 栈大小不是亲和性，也不是完成协议

栈大小建议影响的是实现为线程自动存储等用途准备的平台存储。它不是任务内存预算，不限制堆分配，也不能说明线程已经运行到某个阶段。创建失败仍通过 `std::system_error` 报告，例如资源不足或超过进程线程数限制。

线程属性也不替代平台调度控制。CPU 亲和性、NUMA 放置、优先级、实时策略等仍要按平台 API 或后续标准化工作分别处理。C08 现有 NUMA 题目展示的是平台亲和/内存位置观察，不能当成 P2019R9 的实现证据。

`jthread` 的 stop token 语义不因属性改变。名字和栈建议在创建阶段生效；取消请求、函数返回、异常逃逸、自动 join 仍按 `jthread` 原本契约理解。属性不能让一个不检查 stop token 的函数自动停止。

## 4. 练习检查什么

F01 的 `main.cpp` 只运行教学模型：检查属性必须位于可调用对象之前、重复属性被拒绝、名字是构造期借用、栈大小零值被忽略。它不声称创建了标准 C++29 属性线程。

F01 的 `solution.cpp` 包含真实标准主体：只有 `CS_HAS_STD_THREAD_ATTRIBUTES=1` 时才实例化 `std::thread::name_hint<char>`、`std::thread::stack_size_hint` 和 `std::jthread` 属性构造；否则返回 77。未来本机库支持后，这个主体错误会是 FAIL，不再被 SKIP 掩盖。

## 5. 来源

- [N5055 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)：P2019R9 已应用到 N5054 工作草案。
- [P2019R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p2019r9.pdf)：定义 `thread::name_hint<char>`、`thread::stack_size_hint`、属性前缀参数扫描、重复属性限制、创建失败与 `jthread` wording。
