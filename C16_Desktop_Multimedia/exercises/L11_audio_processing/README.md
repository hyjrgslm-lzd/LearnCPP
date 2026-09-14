# L11 audio processing

正文：[L11 增益、混音、RMS/Peak、波形降采样与重采样边界](../../chapters/11-audio-processing.md)。

学生只编辑 `student/solution.hpp`。输入是归一化 float PCM，所有 sample 和 gain 必须是 finite。

任务：

1. `apply_gain_clamped`：有限 gain，输出 clamp 到 `[-1, 1]`。
2. `mix_clamped`：同长度逐点相加并 clamp。
3. `measure`：计算 RMS 和 peak。
4. `reduce_minmax`：按 bucket 输出 min/max，保留尾部。
5. `resample_linear`：用 `x = i * (input_size - 1) / (output_count - 1)` 做端点对齐插值；单输入样本重复到请求输出长度。
6. 拒绝 NaN/Inf。

有效检查：Reference/good 必须通过；bad 会因不 clamp、RMS 错误、重采样常数/长度错误、非有限值处理错误被拒绝。
