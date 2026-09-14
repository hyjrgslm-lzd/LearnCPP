# L12 video frames

正文：[L12 QVideoFrame、平面、stride、映射寿命与颜色范围](../../chapters/12-video-frames.md)。

学生只编辑 `student/solution.hpp`。已提供 `QImage` backed packed frame、`QVideoFrameFormat::Format_YUV420P` 多 plane frame、RGB888 padding frame 和 limited-range YUV 数字参考。

任务：

1. `inspect_frame` 必须 `map(ReadOnly)`，读取 size、plane count、plane0 stride/mapped bytes、timestamp，返回前 `unmap()`。
2. 无效或不可映射 frame 返回 `valid=false`。
3. 不硬编码 `width * 4`；stride 和 plane count 由 frame 提供。
4. 实现 BT.601 limited YUV 到 RGB 的整数近似并 clamp。

有效检查：packed RGBA、multi-plane YUV420P、padded RGB888、invalid frame、limited black/white/red 方向都会被检查。bad 会因单 plane/硬编码 stride/不 unmap/错误 YUV range 被拒绝。
