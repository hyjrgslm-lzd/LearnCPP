# L12 QVideoFrame、平面、stride、映射寿命与颜色范围

视频帧不是 `width * height * 4` 的裸数组。真实 `QVideoFrame` 可能有多个 plane，行 stride 可能大于可见宽度，像素格式可能是 RGB、YUV 或 GPU-backed 句柄。你只有在成功 `map()` 后，才能把 CPU 可读地址当作内存访问。访问完成必须 `unmap()`。

## 1. stride 与 plane

RGB packed 格式通常一个 plane；YUV 4:2:0 常见三个 plane 或两个 plane。每个 plane 都有自己的 `bytesPerLine()` 和 `mappedBytes()`。行尾 padding 不是像素。遍历时应按 stride 走行，而不是按可见宽度连续读完整图。

练习只创建 `QImage` backed frame 来保证本机可映射，但检查仍要求读取 Qt 返回的 stride 和 plane count，而不是硬编码 `width * 4`。`bad` 变体故意硬编码并忘记 unmap。

一个正确的 CPU 读取循环长这样：

```cpp
if (frame.map(QVideoFrame::ReadOnly)) {
    const int stride = frame.bytesPerLine(0);
    const uchar* base = frame.bits(0);
    for (int y = 0; y != frame.height(); ++y) {
        const uchar* row = base + y * stride;
        // 只在本行可见像素范围内解释 row[x]，不要跨过 stride padding。
    }
    frame.unmap();
}
```

这段代码仍然不完整，因为它还没解释 pixel format。对于 RGBA8888，4 个字节是一像素；对于 NV12，Y plane 与 UV plane 分开，UV 的尺寸和采样率都不同。教学检查不让你写通用转换器，只要求你把“plane/stride 是 runtime metadata”这件事落进代码。

### 可运行观察

`checks.cpp` 用 `QImage(3, 2, QImage::Format_RGBA8888)` 构造 `QVideoFrame`。可见宽度是 3，紧密 RGBA 至少需要 12 字节一行，但实际 `bytesPerLine(0)` 由 Qt 返回。检查只断言 `>= 12`，不把当前机器的对齐值写死。调用 `inspect_frame` 后马上检查 `!frame.isMapped()`，这证明实现没有把映射状态泄漏给调用方。

## 2. 映射寿命

`QVideoFrame` 是共享缓冲句柄。复制 frame 不等于复制全部像素。映射期间拿到的指针只在映射有效期内可用；`unmap()` 后不能继续保存裸指针。若 `map()` 失败，不能退而求其次猜内存布局。

本章 `inspect_frame(QVideoFrame&)` 返回纯值 `FrameInfo`，不返回 `uchar*`。这是故意设计：教学检查只验证 metadata 和可读性，不鼓励把短期映射指针泄漏到外层。

常见错误是把 `QVideoFrame` 当成 `QImage` 的别名：保存 `bits(0)` 指针、把 frame 复制给另一个对象、原函数返回后继续读。这个错误在 CPU-backed 小图上可能暂时“看起来能用”，到 GPU-backed frame、后端复用缓冲或下一帧覆盖内存时就会变成悬空读。本题的接口用值返回切断这条路。

## 3. RGB/YUV 与 limited range

许多视频 YUV 使用 limited range：Y 的黑白范围约为 16..235，UV 中点 128。把它当 full range 会让黑场发灰、白场不满，或颜色偏移。练习实现 BT.601 limited-range 的整数近似：

```text
C = Y - 16, D = U - 128, E = V - 128
R = clamp((298*C + 409*E + 128) >> 8)
G = clamp((298*C - 100*D - 208*E + 128) >> 8)
B = clamp((298*C + 516*D + 128) >> 8)
```

这不是完整色彩管理。真实媒体还要看 color space、transfer function、range、HDR 元数据和渲染路径。课程只用它证明“范围和矩阵是契约，不是随手取三个字节”。

反例：如果把 limited Y=16 当 full-range 0..255，黑色会变成可见灰；如果忽略 UV 中点，红色和蓝色会互相串。练习检查三个点：limited black 近 0、limited white 近 255、一个红色方向样本的 R 大于 G/B。它不证明所有色彩空间正确，只证明你没有把 range 与 chroma 当成无关字段。

## 4. 练习

位置：`C16_Desktop_Multimedia/exercises/L12_video_frames`。

Part：

1. 对有效 `QVideoFrame` 调用 `map(ReadOnly)`，读取 size、plane count、stride、mapped bytes、时间戳。
2. 返回前必须 `unmap()`。
3. 无效或不可映射 frame 返回 `valid=false`。
4. 实现 limited YUV 到 RGB，验证黑、白和红色方向。

## 5. 解析

正确实现用 RAII guard 最稳：构造时 map，析构时 unmap。即使中间早返回，也不会留下 mapped frame。检查器在调用后验证 `!frame.isMapped()`。

`bad/solution.hpp` 的两个错误对应真实项目事故：第一，它硬编码 `width * 4` 当 stride；第二，它 map 后不 unmap。前者在有 padding 或多 plane 时读错行，后者会让后端认为 CPU 仍持有缓冲，影响复用或后续 map。正确实现只把稳定值拷贝到 `FrameInfo`，不把映射指针暴露出去。

源码阅读入口见 [references/standards-and-implementations.md](../references/standards-and-implementations.md) 的 `QVideoFrame` 公开文档和 FFmpeg video renderer。阅读问题：`QVideoFrame` 的 `startTime/endTime` 属于媒体时间还是主机墙钟？renderer 在 seek 后如何避免旧 frame 继续显示？
