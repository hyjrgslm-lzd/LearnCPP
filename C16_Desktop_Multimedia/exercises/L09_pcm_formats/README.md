# L09 PCM formats

正文：[L09 PCM、采样帧与时间换算](../../chapters/09-pcm-time.md)。

学生只编辑 `student/solution.hpp`。已提供 `checks.cpp`、`reference/solution.hpp`、独立 `good/solution.hpp` 和故意错误的 `bad/solution.hpp`。构建目标由 `c16_exercise(c16_l09 QT Core EXPECT "check failed:")` 生成。

任务：

1. 实现 `bytes_per_frame`、字节/frame/time 的安全换算。
2. 拒绝不完整 PCM frame 尾部。
3. 拆分 stereo interleaved `int16_t` 左右声道。
4. 在乘法前检查溢出。

有效检查：Reference/good 必须通过；bad 会因把 sample size 当 frame size、先除后乘、忽略尾帧而被拒绝。Student 初始实现有限失败，不应 SKIP。
