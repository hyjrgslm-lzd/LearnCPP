# L03 integer boundaries

目标：把“整数能放进去”和“表达式暂时算出来”分开。编辑 `src/student/checked_int.hpp`，实现 `checked_cast_u32`。

Part 1：先比较 `std::uint64_t` 输入是否大于 `std::numeric_limits<std::uint32_t>::max()`。失败返回 `c05::Errc::out_of_range`，不能先 `static_cast` 再判断。

Part 2：成功时再窄化为 `std::uint32_t`。检查器会覆盖 0、最大值和最大值加一。

检查目标：`L03_integer_boundaries_reference`、`L03_integer_boundaries_validation_good`、`L03_integer_boundaries_validation_bad_rejected`；开启 student preset 后还会运行学生占位。

Reference 在 `src/reference/checked_int.hpp`。`validation/good` 是独立可通过实现，`validation/bad` 演示“直接窄化会回绕”的常见错法。解析重点：边界判断必须发生在窄化前，否则 `max + 1` 已经变成 0，错误证据丢失。
