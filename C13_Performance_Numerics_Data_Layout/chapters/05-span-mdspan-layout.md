# 05 span、mdspan 与布局契约

先修：C02 的对象生命周期与借用、C06 的连续容器和 view、C13 前面的随机输入与误差预算。本章讲的是接口形状：当函数不拥有数据时，怎样把“这是一段连续内存”“这是二维矩阵”“这是某种 stride 布局”说清楚。

配套 [L03 练习](../exercises/L03_tiled_matrix/README.md) 和 [矩阵公共实现](../exercises/include/c13/matrix.hpp) 使用 `std::span` 与 `std::mdspan`。本课不自造矩阵框架；矩阵所有权仍由 `std::vector<double>` 或调用者管理。

## span 表达一维借用

`std::span<T>` 是指针加长度。它不分配、不释放、不延长生命周期。函数拿到 span 后，只能假设从 `data()` 开始有 `size()` 个连续元素可访问：

```cpp
double sum(std::span<const double> xs);
```

`span<const double>` 表示函数不能改元素，不表示底层存储不可变。其他拥有者仍可能修改同一块内存；并发修改还会进入数据竞争问题。C13 的练习默认单线程，借用只讨论生命周期、长度与别名，不讨论同步。

span 不携带二维 shape。把 `12` 个元素解释成 `3x4` 还是 `2x6`，必须由额外参数说明。额外参数一旦出现，就要检查乘法是否溢出、乘积是否等于真实长度。

## mdspan 表达多维解释

`std::mdspan` 把一段已有存储解释成多维索引空间。C13 使用：

```cpp
using matrix_view =
    std::mdspan<double, std::dextents<std::size_t, 2>, std::layout_right>;
```

`std::dextents<std::size_t, 2>` 表示两维大小在运行期给出。`std::layout_right` 表示右侧索引连续，对二维矩阵就是 row-major：`m[i, j]` 对应线性下标 `i * cols + j`。

这仍然是借用。`mdspan` 不知道底层 vector 是否还活着，也不知道调用者是否把同一段内存同时作为输入和输出传入。它只让 shape、layout 和索引表达更明确。

## shape 要在写入前检查

矩阵乘输入是 `a(ar, ac)` 与 `b(br, bc)`。只有 `ac == br` 才能相乘，输出形状必须是 `ar x bc`。此外，`ar * ac`、`br * bc`、`ar * bc` 都可能溢出 `std::size_t`。若先计算溢出的乘积再比较长度，就可能把错误 shape 误判成合法。

公共函数使用 `c13::checked_product` 先拒绝溢出，再比较真实 span 长度。对写入接口，还先验证输出 shape 和重叠关系，再进入任何写循环。失败前不部分写入，是练习契约的一部分。

## 重叠不是优化细节

矩阵乘会多次读取输入并累加输出。如果输出与输入重叠，早写出的结果会污染后续读取。除非算法专门设计为 in-place，否则最小正确契约就是拒绝重叠。

`std::span` 与 `std::mdspan` 不自动替你检查这一点。C13 的 `matmul_tiled` 写入重载接收输出 span，先检查输出与两个输入 span 是否覆盖同一地址区间。返回 `std::vector<double>` 的重载自己分配输出，因此天然不与输入重叠。

## stride 是 layout 的一部分

本章只使用连续 row-major。真实库还会遇到列主序、子矩阵、padding、转置 view 和自定义 accessor。`mdspan` 的价值正在于这些布局能进入类型或 mapping，而不是在每个循环里靠手写下标暗中约定。

课程现在不扩展自定义 layout，因为 L03 的目标只是矩阵乘和分块尾部。等进入 `std::linalg`、BLAS 或 GPU 内存布局时，再把 stride、leading dimension、accessor 和设备地址空间作为独立契约讲清。

## 练习映射

L03 的 Reference 使用公共 `std::mdspan` view。good 独立写朴素三重循环，但复用同样的 shape 溢出检查。bad 只返回零矩阵，能通过大小但不能通过数值。检查器覆盖非整块尾部、错误 shape、`tile == 0`、乘法溢出与公共写入接口的重叠拒绝。

通过本章后，读者应该能回答：`span` 与 `mdspan` 分别表达什么；为什么 shape 乘法必须先防溢出；为什么输出重叠不能默许；为什么 row-major 是布局契约而不是注释。
