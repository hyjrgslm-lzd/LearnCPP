# L09 PCM、采样帧与时间换算

桌面媒体应用看到的“音频长度 1 秒”不是一个抽象标签。底层缓冲区通常是一段 PCM：按固定 sample rate 采样，按 channel 交织，按 sample format 存成字节。播放、波形、seek、缩略预览和音视频同步，都要把“字节数、采样帧、时间”互相换算。换算错一处，轻则波形偏移，重则缓冲尾部被当成完整帧读出。

本章只讲 PCM 的确定部分，不讲压缩解码。压缩格式进来后，Qt/FFmpeg 先把它解码成某种音频格式；从那以后，你面对的是 sample、frame、channel 和时间。后续 L14 会用 `QAudioBufferOutput` 观察真实解码 buffer，本章先把数学契约固定住。

## 1. sample 与 frame

sample 是一个声道在一个采样时刻的数值。frame 是同一个采样时刻所有声道的组合。mono 16-bit PCM 一个 frame 是 2 字节；stereo 16-bit PCM 一个 frame 是 4 字节。

```text
stereo s16 interleaved:
frame 0: L0 R0
frame 1: L1 R1
frame 2: L2 R2
```

所以字节换 frame 不能只除以 `bytes_per_sample`，必须除以 `channels * bytes_per_sample`。如果缓冲区字节数不是 frame size 的整数倍，尾部就是不完整 frame。正确处理是拒绝或保留到下一段合并，不能静默截断后声称时间完整。

## 2. 交织与声道

交织格式让同一采样时刻的各声道相邻。读取 stereo 时，偶数 sample 是左声道，奇数 sample 是右声道。波形显示常只画 mono 或混合后的能量，但实现不能忘记原始声道数。后续 L11 会把声道混合成处理信号，本章只要求能安全拆出左右声道并拒绝半个 stereo frame。

## 3. 时间换算

sample rate 是每秒 frame 数。48 kHz mono 和 48 kHz stereo 都是每秒 48000 个 frame；差别在每个 frame 的字节数。时间换算应先乘后除：

```cpp
microseconds = frames * 1'000'000 / sample_rate;
frames = microseconds * sample_rate / 1'000'000;
```

先除后乘会把小于 1 秒的片段截成 0。`441 frames @ 44100 Hz` 是 10 ms；如果写成 `(441 / 44100) * 1000000`，结果就是 0。

另一个边界是溢出。长音频和高采样率下，`frames * bytes_per_frame`、`frames * 1'000'000` 都可能超过 64-bit。教学实现用 `uint64_t` 并在乘法前检查上界。真实工程也常把时间保存在 `qint64` 微秒或纳秒里，同样要说明可表示范围。

## 4. 与 Qt 的关系

`QAudioBuffer` 给出 `format()`、`frameCount()`、`sampleCount()`、`byteCount()`、`startTime()` 和 `duration()`。这些值来自解码后 buffer，不等于物理扬声器已经播放到那个位置。`QAudioBufferOutput` 文档也说明它输出的是解码音频观察点；设备输出还有自己的缓冲与延迟。L14 会验证 fixture 确实产生 decoded audio buffer，但不会把 buffer 回调主机时间写成“听到声音的时间”。

## 5. 练习

位置：`C16_Desktop_Multimedia/exercises/L09_pcm_formats`。

学生只编辑 `student/solution.hpp`。公开接口：

```cpp
struct PcmFormat { uint32_t sample_rate; uint16_t channels; uint16_t bytes_per_sample; };
uint64_t bytes_per_frame(PcmFormat);
uint64_t frames_for_bytes(PcmFormat, uint64_t bytes);
uint64_t bytes_for_frames(PcmFormat, uint64_t frames);
uint64_t duration_us_for_frames(PcmFormat, uint64_t frames);
uint64_t frame_floor_for_time_us(PcmFormat, uint64_t us);
StereoSplit split_stereo(vector<int16_t> const&);
```

Part：

1. 计算 stereo s16 的 frame size、字节数和 frame 数。
2. 拒绝不完整 frame 尾部。
3. 正确拆分 stereo 交织数据。
4. 用先乘后除完成时间换算。
5. 对乘法溢出抛出 `overflow_error`。

`reference` 是标准实现；`good` 是独立实现；`bad` 故意把 frame size 当成 sample size，并先除后乘。检查器会拒绝这种常见错误。

## 6. 解析

最小正确实现没有复杂抽象。先验证 `sample_rate/channels/bytes_per_sample` 都非零，再把 `channels * bytes_per_sample` 当作唯一 frame stride。所有后续换算都经过它。尾部策略明确：本题拒绝不完整 frame；真实流式读取也可以缓存尾部，但必须把“还没形成完整 frame”写成状态，不能当作静音或成功。

源码阅读可从 `QAudioBuffer` 与 `QAudioFormat` 开始，再对照 [references/standards-and-implementations.md](../references/standards-and-implementations.md) 中的 `QAudioBufferOutput` 入口。阅读问题：`frameCount()` 与 `sampleCount()` 在多声道下为何不同？`startTime()` 是媒体时间还是设备播放时间？
