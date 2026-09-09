# 并行算法 01：一份执行许可，四组限制

先读[测量先修](../../chapters/12-measurement.md)以及线程的异常和生命周期章节。本篇代码在 [numeric_kernels.hpp](../../exercises/include/concurrency_study/numeric_kernels.hpp) 的 `map`、`sort_values`，完整答案在 [L1/solution.cpp](../../exercises/L1_par_algorithms/solution.cpp)。这里把“可并行”与“本次真的用了多少线程”分开。

## 1. 从没有共享状态的变换开始

输入 x 是整数值 float，限制在 [-16,16]，输出为 `x*x+2`。这些小整数及结果都能被 binary32 精确表示，所以我们可以用整数表达式生成独立真值，不把另一个浮点循环当成唯一裁判。

```cpp
inline float map_value(float x) noexcept { return x * x + 2.0f; }
std::transform(a.begin(), a.end(), out.begin(), map_value);
```

每个输出元素只依赖同位置输入，输出数组已分配，函数体不分配、不加锁、不访问其他元素，不读写全局状态。这给了实现安排调用的自由。先确认这些条件，再选择策略；不要先贴上 `par_unseq`，然后用锁逐个修补共享状态。

本课程的入口检查长度相同、输出与输入不重叠。标准库某些 transform 形式允许完全原地操作，但部分重叠有额外限制；这里选择更简单的**课程合同**，以便所有手写、SIMD、标准算法版本共享同一不重叠约束。输入 span 仍必须来自真实存活的对象，运行时长度检查无法验证一个悬空指针。

## 2. 执行策略改变哪些许可

| 策略 | 元素调用所在执行线程 | 同线程调用之间 | 应用端必须承担的限制 |
|---|---|---|---|
| 无策略 plain | 通常由普通算法调用完成 | 按普通算法自身规定 | 异常按普通重载约定传播 |
| seq | 调用线程 | indeterminately sequenced | 不可依赖未规定的遍历顺序 |
| par | 调用线程或实现创建的线程 | 同一线程上调用不交错 | 共享访问要同步，不等待另一元素推进 |
| unseq | 调用线程 | 可以 unsequenced | 不执行 vectorization-unsafe 操作，不依赖调用先后 |
| par_unseq | 调用线程或实现创建的线程 | 可以 unsequenced | 同时满足并发安全与向量化安全 |

这里的“可以”是许可。实现可以因为输入太小、资源不足或后端选择而串行执行 par；也可以发现函数不适合向量化而不向量化 unseq。反过来，seq 的调用语义也不等于关闭所有机器码层面的向量优化。只要符合可观察行为，编译器仍能做 as-if 优化，所以不能从策略名字推出最终汇编。

“调用不交错”也不等于“按下标从左到右”。对普通 transform 和 for_each，都应以具体算法的规范为准；尤其不要把执行策略当成排序日志的手段。如果需要日志按输入顺序出现，应先把结果写入各自位置，算法返回后再由主线程顺序输出。

标准没有给这个 par 接口一个通用的线程数量参数。`hardware_concurrency()` 是提示，可能为零，也不说明实现最终用了多少线程。课程基准因此把 par 的线程字段写成 0，并在 details 中解释“实现控制、未测”。只有手写 `parallel_chunks` 的路径能报告明确的 worker 数。

## 3. 为什么“加锁就安全”不够

考虑把元素函数写成“取得 mutex，更新共享统计，再释放”。在 par 下，只要没有其他错误，这种同步可能正确，但序列化临界区可能吞掉收益。在 unseq 或 par_unseq 下，还要考虑同一线程内元素调用交错：一次调用拿着锁，另一次调用尝试获取同一把非递归锁，前一次又可能要等交错调用完成才能继续。违反向量化安全要求的标准库同步调用不能靠“实际这次没死锁”合法化。

再考虑一个元素等待另一个元素设置标志。即使使用 atomic 避免了数据竞争，实现也不保证安排那个元素在足够早的时候执行。算法可能整个串行回退，当前元素等不到后面的元素，就永久阻塞。进展依赖和数据竞争是不同问题。

这些错误案例在本文只作为隔离的推理题，默认程序不会执行，不用 sleep “增加成功概率”。本批次正常运行的函数体是预分配输出上的纯数值变换。

不能把规则压缩成“unseq 禁止任何分配”。固定 N5050 的 vectorization-unsafe 定义以及分配/释放的例外需要按具体调用核对；一些分配操作有规范例外，用户自定义分配器或间接调用仍可能做不安全的同步。教学函数统一选择不分配，目的是让合同容易验证，不是假称所有分配一概是 UB。

## 4. 异常行为并不是 plain 的同义替换

无策略重载中的普通用户回调抛异常，通常按该重载的异常约定传播，L1 Reference 用普通 for_each 检查这一点。但使用标准执行策略的算法，用户调用抛出的未捕获异常会触发 `std::terminate`；**seq 也在其中**。算法自身分配临时存储失败可能以 `bad_alloc` 报告，不能把这类实现分配失败与“回调中抛出了 bad_alloc”混为一谈。

因此，不能在 par 回调里调用会失败抛出的 `cs::check`，然后期待 main 的 try/catch 接住。本例先在主线程验证尺寸和形状，回调只执行定义域内的 noexcept 算术，算法返回后逐项检查结果。要处理可恢复的业务错误，可以让每个元素将错误状态写入自己的预分配槽位，再在算法返回后汇总；这是一种数据协议，需要给状态对象同样明确的所有权。

手写 jthread 版本则有另一份异常合同：worker 内部 catch，把 exception_ptr 放进该 worker 独占的槽位，join 后由主线程 rethrow。这能传播异常，是我们实现的行为，不能把它投射到标准 par 重载上。[Cap3](04-parallel-compute.md)会比较这两条路线。

## 5. 不同算法还各有自己的要求

transform 把结果写到与输入下标对应的位置，执行顺序可以不同，输出位置关系不变。for_each 的可调用对象可以修改它拿到的元素，但不能以为所有调用共享同一个函数对象实例，因此把普通成员字段当全局统计也不可靠。L1 的 for_each 只把每个独立 int 乘二。

sort 需要比较关系满足严格弱序。对纯整数值，非递减排序后的值序列和每个值的重数明确，Reference 可以与手写期望数组比较。对带 key 和 payload 的记录，两个相同 key 的对象可互换：`std::sort` 不保证稳定，不能要求它们与另一个 sort 的逐字节顺序一致。若要求同 key 保持原输入顺序，应该选择满足稳定语义的算法并重新比较。

浮点 NaN 会使常用 `<` 比较的严格弱序假设出问题。要排序可能含 NaN 的记录，应先定义 NaN 与普通数的顺序，并为比较器证明要求；本例使用有界整数，避开了这个未定义排序需求。

## 6. Reference 与基准怎样运行

从 exercises 目录执行：

```powershell
cmake -S L1_par_algorithms -B build/l1 -G "Visual Studio 18 2026" -A x64
cmake --build build/l1 --config Release
ctest --test-dir build/l1 -C Release --output-on-failure
```

Reference 对每个可用策略检查空、单元素、短尾和 4097 个元素；对排序检查手写结果和重数，对 for_each 检查独立输出。缺策略能力时仍运行普通算法，并输出跳过信息。`CS_HAS_PARALLEL_ALGORITHMS` 是构建探测结果，不是线程利用率测量。

[L3](../../exercises/L3_par_vs_seq_bench/README.md)重复使用同一 `map`，并提供 `--workload light|heavy`。light 是小整数 x*x+2；heavy 是 double 累积的 96 阶几何多项式，用独立闭式及截断界判分。五种策略分别在 1024、65537、1048577 三档运行完整五样本，具体推导、命令与解释见[轻/重负载与规模正文](05-light-heavy-scaling.md)。默认 size=65537 只是单次运行的方便值，不替代跨规模实验；每一组仍使用相同内核、输入、计时边界和外部随机采样。

## 自测与答案

**seq 能保证不会出现 SIMD 指令吗？** 不能。它约束元素调用的语义，编译器仍可进行保持行为的向量优化。验证实际机器码需诊断与反汇编。

**par 回调中写一个不同下标的数组元素，是否必须加 mutex？** 若输出对象确实互不重叠、每位置仅有一个写者，且寿命覆盖算法执行，则不需要。vector<bool> 的位代理等特殊存储不能按普通独立 int 元素推理。

**在 par 回调中抛出的异常能被调用点 catch 吗？** 标准策略下未捕获回调异常触发 terminate，不能用 plain 的传播行为类推。应预先验证，或使用独占状态槽返回业务错误。

**par 会不会退回单线程？** 可以。因此等待另一个元素推进的协议不成立；耗时或 hardware_concurrency 也不能单独证明本次并行度。

**同 key 记录排序后 payload 顺序不同，一定是 bug 吗？** 非稳定 sort 没有承诺保留同 key 顺序。检查器必须验证实际合同，不能附加不存在的保证。

## 规范与实现资料

- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[algorithms.parallel.exec]`、`[algorithms.parallel.exceptions]`、`[alg.transform]`、`[alg.sorting]`。这是本篇 C++26 对照锚点。
- [Microsoft 自动并行化与向量化](https://learn.microsoft.com/en-us/cpp/parallel/auto-parallelization-and-auto-vectorization?view=msvc-170)：编译优化的实现行为与库策略许可应分别记录。
