# 00 执行与对象：先回答“谁还活着”

假设你已经会写一个返回整数的函数，现在想把它放进一个任务，稍后再调用。这个“稍后”会改变什么？函数体可能完全不变，真正变化的是：调用时输入对象是否还存在、任务保存的是副本还是引用、异常路径由谁清理资源。线程只是把这些问题放大；即使只有一个线程，返回一个引用了局部变量的闭包也可能出错。

本章先不创建线程。[P1 Starter](../exercises/P1_object_lifetime/main.cpp)给出最小观察，[完整 Reference](../exercises/P1_object_lifetime/solution.cpp)的六个 `part` 函数验证寿命、捕获与移动、统一调用、异常清理，以及用 tuple 保存参数、稍后展开调用。默认编译标准为 C++23；本章不依赖 C++26 新特性。后文谈规范时以 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 为 C++26 固定检查基准，eel 链接仅用作可搜索的滚动条款导航，不能把其未来变化直接当作 C++26。

## 1. 名字、对象、存储和资源不是同一件事

`int value = 10;` 创建一个对象，`value` 是我们在作用域内访问它的名字。`int& alias = value;` 建立别名，没有复制一个新整数；`int copy = value;` 才创建另一个整数。随后写 `alias = 20` 会改变原对象，`copy` 仍然是 10。引用本身不能让原对象一直存在。

再看 `auto owner = std::make_unique<int>(42);`。这里至少要区分两个对象：局部的 `unique_ptr` 句柄，以及动态分配的整数。句柄负责在释放所有权时销毁整数、归还存储。句柄可以被移动；被管理整数通常不需要跟着移动地址。这就是后面“把输入交给任务”的基础：移动所有权与复制数据是两种不同操作。

对象寿命也不等于“那块地址看起来还没被覆盖”。对本章这种普通、非平凡类对象，成功构造建立了可正常使用的对象；析构开始时其寿命进入结束过程，析构期间还有专门规则。存储以后仍可能存在，但不能因此继续通过原对象执行普通访问。更一般的隐式对象创建、存储复用和 `std::launder` 属于后续生命周期专题，不用来为悬垂引用开脱。依据见 [对象寿命条款](https://eel.is/c++draft/basic.life)，固定版定位为 N5050 `[basic.life]`。

P1 的 `scope_object` 用一个外部记录表保存事件。先构造 `first(1)`，再构造 `second(2)`，离开作用域后记录必须是 `1, 2, -2, -1`。记录表比两个对象先构造，因此比它们晚析构，析构函数写入它时仍然有效。这个细节和实验结论同样重要：观察工具本身也要服从对象寿命规则。

Reference 使用固定四项数组记录这四个事件，析构函数不分配内存、不输出日志、不抛异常。它是针对两个对象的观察器，不能直接当成任意长度的日志容器。真正的打印发生在作用域结束之后，检查通过才输出解释。

## 2. 把函数保存起来，就必须决定保存什么输入

P1 Starter 的核心代码是：

```cpp
int value = 10;
auto snapshot = [value] { return value; };
auto borrowed = [&value] { return value; };
value = 20;
```

此时 `snapshot()` 得到 10，`borrowed()` 得到 20。按值捕获为闭包保存了一份整数；按引用捕获让稍后的调用继续访问原整数。闭包本身是一个对象，保存闭包的变量也有自己的寿命。

Reference 把 `snapshot` 改成 `[value]() mutable { return ++value; }`。`mutable` 允许修改闭包保存的那份值，调用后得到 11，外面的 `value` 仍为 20。它不代表“给原对象解除 const”，也不提供任何线程同步。引用闭包递增原变量后，外部值才变为 21。规范入口为 [lambda 捕获](https://eel.is/c++draft/expr.prim.lambda.capture)，固定版定位 `[expr.prim.lambda.capture]`。

这时很容易提出一个看似合理的改动：把 `borrowed` 返回给外层，等函数返回后再调用。问题在于被引用的 `value` 已经析构，闭包保存下来的访问路径不能延长其寿命。本课程不运行这段错误代码；对应的安全方案是让闭包拥有值，或者让调用方明确保证被引用对象活到最后一次调用结束。

按值捕获也不是“深复制所有东西”。捕获一个裸指针，只复制地址；捕获 `string_view` 或 `span`，只复制视图；捕获 `shared_ptr` 会增加共享所有权，但不会给其指向对象的读写加锁。`[this]` 保存的是指针，外层对象仍需存活；`[*this]` 则按相应复制语义保存对象副本。审查异步闭包时必须顺着成员继续追踪，不能在看到 `[=]` 时停止。

## 3. std::move 是一次许可，所有权变化由类型决定

`std::move(x)` 本身主要改变表达式的值类别，让后续重载有机会选择移动操作。它不会独立创建线程，不会删除 `x`，也不保证某个构造一定没有复制。真正移动什么，由接收该表达式的构造、赋值或函数调用决定。

P1 使用的是有明确移动后状态的 `unique_ptr`：

```cpp
auto owner = std::make_unique<int>(42);
auto task = [p = std::move(owner)] { return *p; };
```

初始化捕获从 `owner` 转移所有权。此后 `owner` 为空，闭包内的 `p` 拥有整数。检查 `!owner` 合法，再解引用 `owner` 则不合法。Reference 又执行 `auto moved = std::move(task);`，随后只调用 `moved()`，不调用已经交出资源的 `task()`。

不能把“unique_ptr 移动后为空”推广到所有类型。许多标准库类型移动后有效但值未指定；只能执行该类型允许的操作，不能用“所有 moved-from string 必为空”作为可移植检查。对本课程会用到的 `future`、`thread`、`packaged_task`，各自还有明确的移动后关联状态，必须逐个学习。

独占资源还改变了可复制性。拥有 `unique_ptr` 的闭包不能被复制，Reference 用 `static_assert` 固定这个事实。后面的 promise 和 packaged_task 也是只移动类型，所以常见写法是初始化捕获 `[p = std::move(provider)]`。如果闭包需要调用非 const 成员（例如 `p.set_value(...)` 或 `t()`），通常还需 `mutable`。

## 4. std::ref 表达借用，std::invoke 统一调用语法

函数参数按值传入时，函数修改的是参数对象。`std::ref(original)` 产生一个可复制的引用包装器，表达“后续仍访问 original”。包装器可以复制，不代表被引用对象被复制，更不代表其寿命被延长。

P1 的 `counter` 同时包含数据成员 `value` 和成员函数 `add`：

```cpp
std::invoke(&counter::add, std::ref(original), 7);
```

这会调用 `original.add(7)`。`std::invoke` 也接受普通函数对象和 lambda；对于成员指针，则按规则处理对象、指针和引用包装器。Reference 还用 `std::invoke(&counter::value, original)` 访问数据成员。因此它是“怎样调用”的统一入口，不是“在哪个线程调用”的调度器。依据见 [invoke](https://eel.is/c++draft/func.invoke) 和 N5050 `[func.invoke]`。

这一点能直接解释后面的线程参数。把普通整数传给 `jthread`，线程会保存独立参数值；要让 `void f(int&)` 修改外部整数，通常显式传 `std::ref(value)`。此时我们从复制契约切换到了借用契约：必须保证被引用对象存活，而且并发读写遵守同步关系。语法变短并没有减少这两项义务。

## 5. RAII 把清理绑定到已经构造成功的对象

RAII 的关键不是“所有资源都放在栈上”，而是拥有资源的对象负责清理。拥有者可以有自动、动态或成员对象寿命；重点是其析构发生在什么边界。

P1 `part4_unwinding()` 在构造两个观察对象后抛出异常。控制流转移到外面的 handler 之前，这两个完整构造的局部对象依逆序析构，因此仍得到 `1, 2, -2, -1`。如果某个对象自身构造失败，则不会调用那个完整对象的析构，但其已经构造成功的子对象仍按规则清理。这解释了为什么资源应尽早交给拥有者，而不能等函数末尾手写一串 `delete`、`close`、`unlock`。

线程也需要这样的拥有者。`jthread` 在仍关联线程时，析构会请求停止并等待线程结束；我们在第 03 章展开。但 RAII 并不保证析构快速：等待线程结束可能需要很久，也可能因协议错误永远等不到。析构拥有者还可能依赖其他对象，所以声明顺序就是协议的一部分。例如供 worker 借用的输出和错误槽应该活得比 worker 的 join 更久。

## 6. 不同类型的参数一起保存：tuple 究竟拥有了什么

前面的闭包把每个输入分别捕获。现在假设调用接口已经写成 `f(number, text)`，希望把这组参数保存下来，稍后交给执行者。`std::tuple` 可以在一个对象中保存类型各异的元素，但它是否拥有输入，取决于元素类型和构造方式；不能只看容器名字。

P1 的 `part5_tuple_ownership()` 从同一组变量建立三种参数包：

```cpp
int number = 10;
std::string text = "saved";
auto owned = std::make_tuple(number, text);
auto borrowed = std::tie(number, text);
auto unwrapped = std::make_tuple(std::ref(number), std::cref(text));
```

`owned` 的类型是 `tuple<int, string>`，这里保存整数和字符串副本；`borrowed` 的类型是 `tuple<int&, string&>`，保存访问原对象的引用。随后把原变量改为 20 和 `"later"`，前者仍保存 10 和 `"saved"`，后者读取到新的值。Reference 同时检查实际数值与类型，不用仅打印 `typeid` 名称来猜测。

第三行看似仍是 make_tuple，却不是拥有独立数据。`std::ref` 返回 `reference_wrapper<int>`，`std::cref` 返回只读引用包装器；make_tuple 特别将这类包装器解包为引用元素，因此 `unwrapped` 是 `tuple<int&, const string&>`。这个特例允许一组参数中有的按值、有的明确借用。Reference 检查元素地址就是原变量地址，避免把“碰巧值相等”误认为相同对象。

也可以显式保存包装器本身：`tuple<reference_wrapper<int>> wrapped{std::ref(number)}`。此时 `get<0>(wrapped)` 是包装器，调用它的 `.get()` 才取得原整数引用。复制包装器只复制借用关系，不复制整数；对 `.get()` 赋 22 后，number、borrowed 和 unwrapped 都读到 22。

`std::tie` 还常用于向已有变量逐项赋值：`borrowed = std::make_tuple(30, std::string("assigned"));` 将 number 和 text 改为 30、`"assigned"`，没有创建新的引用目标。原 owned 快照仍不变。这里应区分“引用元素的赋值写入所指对象”和“reference_wrapper 自身赋值可以改变包装的引用关系”；P1 使用 `.get()` 修改被引用值。

| 保存方式 | 本例元素类型 | 原变量改变后 | 延后使用的寿命责任 |
|---|---|---|---|
| `make_tuple(number, text)` | `int, string` | 保留副本 | 参数包拥有本例的值 |
| `tie(number, text)` | `int&, string&` | 观察原变量 | 原变量必须活到最后一次调用结束 |
| `make_tuple(ref(number), cref(text))` | `int&, const string&` | 观察原变量 | 包装器解包也不延长原变量寿命 |
| 显式 `tuple<reference_wrapper<int>>` | 包装器对象 | `.get()` 访问原变量 | 包装器存活不代表原变量存活 |

这个“拥有”的结论只针对本例的 int 和 string。make_tuple 若收到指针、string_view 或 span，只会保存相应指针或视图；字符串字面量通常衰减为字符指针，并不会自动构造 string。需要拥有文本时就显式构造 string。移动一个 `tuple<int&>` 也不会把引用变成独立整数，Reference 检查从这个右值 tuple 取得的元素仍是 `int&`。

这些创建规则可在 [N5050 的 tuple 创建条款](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf#page=845) `[tuple.creation]` 核对；[滚动条款导航](https://eel.is/c++draft/tuple.creation)便于搜索。`forward_as_tuple` 也是引用元组，适合立即转发，不能用它把临时参数的寿命延长到“以后”：临时对象消失后，再漂亮的参数包也只剩悬垂路径。

## 7. std::apply 把参数包展开，移动发生在实际调用时

tuple 保存了数据，还没有执行函数。`std::apply(fn, args)` 按元素索引将参数展开，再按 INVOKE 规则调用 fn。对于两个参数，可以把效果理解为 `std::invoke(fn, std::get<0>(args), std::get<1>(args))`，但真正的 apply 还会按传入 tuple 的值类别转发元素。它不会安排线程，也不会因为你刚保存 tuple 就自动调用函数。固定规范入口为 [N5050 `[tuple.apply]`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf#page=846)，亦可查 [apply 导航](https://eel.is/c++draft/tuple.apply)。

P1 `part6_saved_arguments()` 将调用对象和参数分别保存到两个闭包：owned_call 捕获 make_tuple 得到的值，borrowed_call 捕获 tie 得到的引用。保存时 number=10、increment=3，真正调用前改为 20、4；检查调用次数仍为零，再分别调用，结果必须为 13 和 24，调用次数变为二。借用版虽然按值捕获了 tuple 对象，tuple 内部的引用依然指向原变量。闭包本身被复制或移动，不会改变这个事实。

再考虑接收独占资源的函数：`consume(unique_ptr<int> p, int delta)` 要取得指针所有权。若 apply 接收一个左值 `tuple<unique_ptr<int>, int>`，展开的第一个参数是 unique_ptr 左值引用，不能用它复制构造按值形参。P1 用 `is_invocable_v` 检查这个不可调用组合，并确认右值引用组合可调用；错误表达式不进入默认编译路径。

真实可运行的版本把 37 的独占指针和增量 5 保存进 tuple，再把 tuple 移入闭包，最后在闭包调用时执行：

```cpp
auto task = [fn = consume, args = std::move(arguments)]() mutable {
    const int result = std::apply(fn, std::move(args));
    cs::check(!std::get<0>(args), "apply moves element into by-value parameter");
    return result;
};
```

所有权分三次交接：原 unique_ptr → 外部 arguments 的元素 → 闭包 args 的元素 → consume 的按值参数。Reference 逐段检查前一个拥有者为空，最后检查结果为 42。这里 `std::move(args)` 本身只是允许按右值展开，真正转移资源的是按值参数的构造。若被调用函数只接收引用而不移动它，不能凭出现了 std::move 就断言元素已经被消费。

这个闭包需要 mutable，才能从它保存的非 const unique_ptr 元素移出资源；它也因拥有独占资源而不可复制。调用之后输入已被消费，本题只调用一次，不能把一个普通闭包误当成会自动阻止重复执行的状态机。闭包里的检查在消费前确认指针非空，在消费后确认元素为空；默认不执行空指针解引用。

到这里，“保存参数稍后执行”已经有了完整对象图：值参数由任务拥有，借用参数依赖外部寿命，调用对象与 tuple 都只是存储，apply 才执行展开调用。这正是后续 packaged_task、线程参数与任务队列所需的先修，而不是额外发明一个调度器。

## 8. Part：预测、运行、解释

从 P1 的 [README](../exercises/P1_object_lifetime/README.md) 使用构建命令；Starter 展示捕获与保存值参数的基线，Reference 覆盖以下全部必做内容：

| Part | 动手内容 | 对应 Reference | 必须解释的结果 |
|---|---|---|---|
| 1 | 记录两个对象的构造与析构 | `part1_lifetime` | 后构造的局部对象先析构 |
| 2 | 修改捕获值，移动独占资源 | `part2_capture_and_move` | 修改副本不影响原整数；移动后的所有者改变 |
| 3 | 普通调用、成员调用、引用包装 | `part3_invoke` | invoke 执行调用规则，不负责异步调度 |
| 4 | 从内层作用域抛出异常 | `part4_unwinding` | handler 执行前资源已依逆序清理 |
| 5 | 对照 make_tuple、tie 与引用包装 | `part5_tuple_ownership` | 值副本保持不变；引用/包装器沿原对象变化；tie 赋值写回原变量 |
| 6 | 保存调用对象和 tuple，稍后 apply | `part6_saved_arguments` | 保存不执行；拥有/借用得到 13/24；移动独占参数得到 42 |

不要只看退出码。先写下每个对象的拥有者，标出最后一次访问与销毁位置，再比较输出和 `cs::check`。如果改动捕获方式后输出变化，解释应落在“哪个对象被访问”上，而不是“编译器可能优化了”。

## 自测与解析

**把闭包 move 到队列，外部引用就安全了吗？** 没有。移动的是闭包对象；引用捕获仍指向同一个外部对象。必须把被引用对象的寿命覆盖到真正执行结束，或改为拥有值。P1 Part 2 的独占资源版本展示了后者。

**为何 mutable 闭包还可能无法放进 std::function？** mutable 只影响闭包调用的 const 性质。`std::function` 的目标需要可复制；捕获 unique_ptr 或 packaged_task 的闭包不满足这一要求。D3 使用 `std::packaged_task<void()>` 接收这种闭包。若只需只移动的调用包装，C++23 还提供 `std::move_only_function`。依据见 [function 构造要求](https://eel.is/c++draft/func.wrap.func.con)与 [move_only_function](https://eel.is/c++draft/func.wrap.move)。

**一个引用先销毁，是否会销毁被引用对象？** 不会。引用不拥有对象；反过来，引用存在也不会阻止原对象销毁。这正是借用与拥有的区别。

**按值捕获 `std::tie(...)` 的结果，就能返回闭包以后再调用吗？** 只有被引用变量确实活得足够久才行。按值捕获复制的是引用元组，仍然借用原变量；要独立寿命，应为相应输入保存拥有值或资源的元素。

**make_tuple 总是复制吗？apply 总是移动吗？** 都不是。make_tuple 可以复制值、从右值移动，也会将 reference_wrapper 解包为引用。apply 按参数包的值类别展开；元素实际是否移动，取决于被调用函数的参数与函数体。Part 6 用按值接收 unique_ptr 的函数，才明确发生资源消费。

**日志显示析构顺序正确，是否已经证明所有捕获都安全？** 只验证了这次对象结构和输入。还要检查闭包是否逃逸、所指资源是否被其他拥有者提前释放、异常路径是否改变等待顺序。本章只研究单线程，之后仍要为跨线程访问补同步证明。

下一章把“一个对象上的一次调用”扩大成“多个任务之间的执行关系”；带着对象图继续读 [01 并发心智模型](01-concurrency-model.md)。
