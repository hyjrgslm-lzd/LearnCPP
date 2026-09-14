# L06: 固定随机输入与 views 观察

实现 `student::summarize(std::size_t count, std::uint32_t seed)`。

函数生成 `[-2, 2]` 区间内的 `double` 样本，记录前五个非负值，并用稳定归约求这些值的和。随机输入必须由固定 seed 驱动；同一个 seed 重复运行得到同一组样本，换 seed 才是新输入。这里使用 `std::mt19937` 和 `std::uniform_real_distribution<double>`，它们适合可复现实验输入，不是密码随机数。

`c13::non_negative_values` 返回 view。它不拥有样本，也不延长 `std::vector` 的生命周期；遍历时读取的是当前底层存储。检查器会修改一个被借用的 vector，确认 view 观察到新值。

- `student/solution.hpp`：起点忽略 seed，会失败。
- `reference/solution.hpp`：调用公共生成器和 view helper。
- `good/solution.hpp`：独立使用标准随机库与 ranges。
- `bad/solution.hpp`：把调用次数混进 seed，检查器必须拒绝。
