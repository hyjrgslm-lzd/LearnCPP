# L02 数据布局：转换与完整更新

先读 [06 数据布局](../../chapters/06-aos-soa.md)。本题实现与观察分开：`checks.cpp` 使用所选目录里的 `solution.hpp`，`benchmark.cpp` 使用完整课程内核。

## 任务和契约

1. 编辑 `student/solution.hpp` 的 `convert`，将 AoS 的全部字段按原顺序转换为六个等长数组。空输入返回空结构；不得调用 `c13::to_soa` 或参考答案代做。
2. 实现 `advance`：先核对全部长度与有限非负 `dt`，然后更新三个位置字段，保留速度和元素个数，必须处理尾部。错误参数抛出 `std::invalid_argument`，错误发生前没有写入。可以使用提供的 `validate` 和 `validate_dt`。
3. 运行完整布局驱动，预测转换成本是否会抵消内核收益，再记录实际结果与计时范围。运行成功不表示这一解释任务已完成。

主线输入是有限小值；题目不承诺任意 `float` 极值计算都保持有限。位置更新误差按检查器给定的绝对和相对误差预算检查。不要把容差调大当作修复。

## 构建与检查

从课根目录的 MSVC 开发终端运行：

```powershell
cmake -S exercises/L02_layout -B build/layout -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/layout
./build/layout/c13_layout_student.exe
ctest --test-dir build/layout --output-on-failure
```

初始 Student 明确以非零退出；默认 CTest 不把未完成作业注册为通过。Reference/good 必须通过，bad 因漏掉尾部而被检查器拒绝。四个实现都可编译。检查器还验证转换字段、保留速度及形状拒绝，不依赖 Release 会禁用的 `assert`。

## 解析

`reference/solution.hpp` 调用课内完整内核；`good/solution.hpp` 独立写出转换和逐字段更新，不包含 Reference。它们共享接口与检查器，但算法互不调用。完整推导见正文；学习时先遮住这两个目录。

转换时如果只复制位置、不复制速度，位置初始化检查可能通过，但随后的更新必然错误。因此检查器同时检查两个字段组。bad 只更新 `size/4*4` 个元素，构造的是有限、无越界、能稳定失败的语义反例，不靠未定义行为制造随机崩溃。
