# 练习 L04：构造完成与异常展开

先阅读 [04. 构造、析构与异常展开](../../chapters/04-construction-and-unwinding.md)。本练习只验证本章核心规则：构造顺序、析构逆序、第二个资源构造失败时已完成成员会清理，未完成的最外层对象不会清理。

学生只编辑 `src/student/construction_lab.hpp` 和 `src/student/construction_lab.cpp`。`checks`、`reference` 和 `validation` 不属于学生编辑区。

| Part | 操作 | 检查点 |
|---|---|---|
| L04-1 | 实现 `observe_order()` | 返回 `Base()`、`Member first()`、`Member second()`、`Derived body`、`~Derived body`、`~Member second()`、`~Member first()`、`~Base()` |
| L04-2 | 实现一个不可复制的 `ResourceOwner` | 构造时调用 checker 提供的 `Recorder::acquire`，析构时释放自己持有的 handle |
| L04-3 | 用两个成员 owner 实现 `TwoResourceOwner` | checker 在对象作用域内看到 live 为 2，离开作用域后 live 为 0 |
| L04-4 | 让 checker 设置第二次 acquire 抛异常 | 最外层对象未完成，但第一个成员 owner 析构，checker 看到 live 回到 0 |

配置和运行叶级练习：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L04_construction -B C02_Objects_Lifetime_Ownership/exercises/L04_construction/build/c02-foundation-author -G "Visual Studio 18 2026" -A x64
cmake --build C02_Objects_Lifetime_Ownership/exercises/L04_construction/build/c02-foundation-author --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L04_construction/build/c02-foundation-author -C Debug --output-on-failure
cmake --build C02_Objects_Lifetime_Ownership/exercises/L04_construction/build/c02-foundation-author --config Release
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L04_construction/build/c02-foundation-author -C Release --output-on-failure
```

默认 `CORE_STUDY_TEST_STUDENTS=OFF` 时只运行 reference 与公开 good/bad checker 验证。打开学生测试后，starter 会安全编译，但运行明确失败，直到你完成实现。

checker 自己创建 `Recorder`，直接构造你的 `TwoResourceOwner`，再读取 checker 持有的 live 计数和固定事件缓冲。学生代码不能通过返回一份手写统计结果来绕过资源题。`Recorder` 是安全资源模型：它只登记固定数量的资源 id 和事件，不分配真实资源，也不在释放后读取已释放对象。它证明的是构造/析构路径、资源计数和异常展开，不等同于真实堆对象或系统句柄释放已经被实测覆盖。

## 解析

L04-1 证明成员构造顺序来自声明顺序，不来自初始化列表顺序。析构先执行最外层析构函数体，再按成员构造逆序析构，最后析构基类。

L04-2 的 owner 是本章最小 RAII 单元。构造函数从 checker 持有的 `Recorder` 获取一个 handle 后立刻拥有它，析构函数把同一个 handle 交回 `Recorder::release`。复制被删除，因为两个对象同时释放同一个资源会破坏所有权。

L04-3 正常路径中，两个成员 owner 都构造完成。最外层对象作用域内 checker 看到 `live == 2`；离开作用域时先析构第二个成员，再析构第一个成员，最终 `live == 0`。

L04-4 中第二个资源获取抛异常。第二个 owner 没有完成构造，所以它的析构函数不运行；第一个 owner 已经完成构造，所以异常展开会析构第一个 owner 并释放资源。最外层对象没有完成构造，所以它自己的析构函数不运行。检查器用 `live == 0` 防止“只在最外层析构释放”的错误写法漏掉失败路径。
