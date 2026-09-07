# 练习 P-2：generator 的启动、推进与生命周期

先阅读 [预备章的序列与 generator](../../00-预备知识-执行模型与标准库.md#generator-model)。本题只使用三个数字，把注意力放在消费动作和生产者执行之间的关系上。

| Part | 本次操作 | 对应讲解 | 参考结果 |
| --- | --- | --- | --- |
| P2-1 | 依次产出 1、2、3，手动 begin、解引用和递增 | [消费驱动的执行](../../00-预备知识-执行模型与标准库.md#generator-model) | 连读两次得到 1，递增后得到 2、3 |
| P2-2 | 只取第一个值，让 generator 离开作用域 | [暂停与局部对象](../../00-预备知识-执行模型与标准库.md#generator-lifetime) | `yielded=1 continued=0 local_destroyed=1` |
| P2-3 | 将序列收集到 vector，再遍历两次 | [保存结果](../../00-预备知识-执行模型与标准库.md#materialization) | 两次求和都为 6，生产者进入一次 |

`main.cpp` 保留三个作用域与日志位置，按 TODO 完成实验。`solution.cpp` 用计数观察同一过程。这里的析构计数记录协程体局部对象，整个协程状态的释放由 generator 的所有权约定决定。

在 `Coroutine_Study/exercises` 构建：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --target P2_generator_basics P2_generator_basics_reference
```

Visual Studio 默认构建目录中的运行方式：

```powershell
./build/verify-core/P2_generator_basics/Release/P2_generator_basics.exe
./build/verify-core/P2_generator_basics/Release/P2_generator_basics_reference.exe
```

其他 CMake 生成器的可执行文件位置见 [构建指南](../BUILD_GUIDE.md)。Reference 最后输出 `P2_reference OK`。

每次执行前，先预测 `producer entered`、`before yield`、`after yield`、析构日志的顺序。执行后解释预测与观察的差异。

**答案解析：** 创建 generator 后，函数体保持待启动。`begin()` 才打印 `producer entered` 和第一次 `before yield`；连续解引用只读取当前值。每次递增先执行上一个 yield 后的语句，再运行到下一次 yield，所以完成 Part 1 并产出 1、2、3 后，完整消费会观察到三次 `after yield`，随后局部对象析构。

Part 2 只读取首值就离开作用域，日志顺序为进入生产者、第一次 `before yield`、局部对象析构，yield 后的语句保持未执行。Part 3 收集元素时完整推进一次生产者；随后两次遍历 vector 使用的是保存的值，两次求和均为 6。Starter 初始值是三个 0，而且 Part 1 尚未补完递增动作；先按 TODO 完成对应步骤，再对照这些完成版结果，可以准确定位观察差异来自哪项改动。

接着读 [01 心智模型](../../01-心智模型.md)，再到 [A1](../A1_first_generator/README.md) 把三值生成器扩展为 Fibonacci 和文本序列。
