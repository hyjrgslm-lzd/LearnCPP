# 03 随机输入、固定 seed 与 views

先修：C06 的 ranges/view 借用模型、本课第 02 章的数值检查。目标是把随机输入变成可复现的实验条件，而不是让每次运行“看起来更随机”。配套 [L06 练习](../exercises/L06_random_views/README.md) 使用 `std::mt19937`、`std::uniform_real_distribution<double>` 和 `std::views::filter/take`。

## 随机不是不可复现

性能和数值实验经常需要大批输入。手写固定数组太窄，真实业务数据又难以提交进课程仓库。标准随机库给出中间路线：固定 engine、distribution、seed 和样本规模，就能在同一实现上重建同一批输入。

```cpp
std::mt19937 engine(seed);
std::uniform_real_distribution<double> distribution(-2.0, 2.0);
```

这段代码的契约是实验可复现，不是密码安全。`std::mt19937` 是确定性伪随机引擎；知道 seed 和算法就能重放序列。密钥、token、抽奖、公平对抗场景都不该用它。本课只用它生成矩阵、粒子和数值样本。

## seed 是输入的一部分

如果 seed 改了，输入就改了。一次优化在 seed A 上更快，不等于它在 seed B 上也更快；一次数值误差在 seed A 上没暴露，不等于算法稳定。报告实验时至少要写 engine、distribution、seed、样本数和范围。

[公共生成器](../exercises/include/c13/numerics.hpp) 的 `fixed_uniform_input(count, seed, lo, hi)` 把这些选择集中在一个小函数里。它检查 `lo <= hi`，生成 `count` 个有限区间样本。随机数生成通常不放进核心计算计时窗口；若研究的正是生成成本，应单独成章、单独计时。

## distribution 也是契约

同一个 seed 配不同 distribution 会产生不同输入。均匀分布、正态分布、离散分布会改变数值范围、分支行为、cache 局部性和异常路径。例如矩阵乘只看固定顺序累加，粒子管线还会受位置、速度、质量筛选影响。把“随机输入”写进 README 却不写 distribution，等于没有写实验输入。

本章使用 `uniform_real_distribution<double>(-2.0, 2.0)`，因为它能自然产生正负数，让后续 view 过滤有可观察结果。检查器验证所有样本落在范围内，同一 seed 重复结果相同，不同 seed 结果不同。

## views 观察已有存储

`std::views::filter` 和 `std::views::take` 默认不拥有底层元素。它们保存的是 range 与谓词等轻量状态，遍历时再读取源数据：

```cpp
auto non_negative = xs | std::views::filter([](double v) { return v >= 0.0; });
```

若 `xs` 是 `std::vector<double>`，这个 view 不能比 vector 活得更久。若 vector 在 view 创建后被修改，view 遍历时看到的是修改后的值。L06 检查器专门修改一个被借用的 vector，确认它不是提前复制出的结果。

需要拥有结果时，显式收集到 `std::vector`。`first_non_negative_values` 遍历 view 的前 N 个元素并复制出来，返回的 vector 与源数据脱钩。这个边界很重要：view 适合表达管线和避免临时分配，但它不能替代所有权模型。

## 练习映射

L06 的 `student::summarize(count, seed)` 返回三项：完整样本、前五个非负样本、这些非负样本的稳定和。Student 起点忽略 seed，bad 把调用次数混进 seed，两者都会破坏复现性。Reference 调用公共生成器和 view helper；good 独立使用标准库组合。

通过 L06 后，读者应该能解释：为什么 seed 要写入命令或 README；为什么随机生成不默认计入核心算法耗时；为什么 view 能在源 vector 修改后看到新值；什么时候应该把 view 结果复制成拥有型容器。
