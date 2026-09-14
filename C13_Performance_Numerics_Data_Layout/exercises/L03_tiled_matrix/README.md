# L03: 借用矩阵视图与分块乘法

实现 `student::multiply`。

输入是 row-major 连续存储：`a` 的形状是 `ar x ac`，`b` 的形状是 `br x bc`，输出是 `ar x bc`。`ac != br`、输入长度与形状不匹配、`tile == 0`、形状乘法溢出都必须在写输出前拒绝。公共写入接口还拒绝输出与输入重叠。

- `student/solution.hpp`：起点返回空结果，会失败。
- `reference/solution.hpp`：调用公共 `std::mdspan` 分块实现。
- `good/solution.hpp`：独立朴素三重循环，用相同契约检查。
- `bad/solution.hpp`：只分配零输出，检查器必须拒绝。

检查器覆盖 `2x3 * 3x2` 基本结果、`3x5 * 5x4` 的非整块尾部、shape 错误、tile 错误、size 乘法溢出和公共写入接口的重叠拒绝。分块大小是实验参数，不是正确性前提。
