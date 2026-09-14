# L14 QMediaPlayer、QAudioBufferOutput 与 QVideoSink 真实解码

前几章建立了 PCM、队列、处理、frame 映射和时钟模型。本章把固定 fixture 交给 Qt Multimedia，用公开 API 观察真实解码：`QMediaPlayer` 读取文件，`QAudioBufferOutput` 收到 decoded audio buffer，`QVideoSink` 收到 decoded video frame。

本章不要求扬声器设备。我们不设置 `QAudioOutput`，只用 audio buffer output 观察解码结果。缺声卡不能让解码测试 SKIP；只有后端无法解码固定文件才是当前环境的真实失败，需要记录。

另有一个独立设备验收入口 `c16_audio_device`。它不参与四个解码 variant，也不替代 fixture oracle。该目标使用 `QMediaDevices::defaultAudioOutput()` 和 `QAudioSink`，按默认设备的 `preferredFormat()` 写入约 100ms 静音，`volume=0`，并验证 sink 能启动、接受数据、`processedUSecs()` 前进和 `stop()` 收尾。只有没有默认输出设备时返回 77，让 CTest 记为 SKIP；启动失败、写入失败或进度不前进都是 FAIL。这个检查只证明真实默认音频输出管线可打开和消费静音，不证明扬声器真的发声，也不测端到端延迟，不修改系统设备设置。

超时要单独分类。`probe_media()` 暴露 `timed_out`，任何 timeout 都是测试失败；坏输入必须由 `QMediaPlayer` 的真实错误或 `InvalidMedia` 证明，不能把“等不到结果”当成成功拒绝。实现还要处理 `setSource()` / `play()` 后同步完成或同步报错的情况：如果信号已经把 probe 标记为完成，就不要再进入局部事件循环，避免 `quit()` 早于 `exec()` 被丢掉。

## 1. 观察点边界

`QAudioBufferOutput::audioBufferReceived` 收到的是解码音频 buffer。它能证明 fixture 被后端解析并产生音频 frame，能读取 sample rate、channel count、frame count、buffer start time。它不能证明物理扬声器在同一时刻播放。

`QVideoSink::videoFrameChanged` 收到的是视频 frame。它能证明视频解码和帧交付，能读取 size 和 frame timestamp。它不等于窗口已经完成合成；本章不测渲染器 vsync 或显示延迟。

高层同步由 `QMediaPlayer` 后端承担。L13 的时钟模型只验证教学契约，不替 Qt 内部 time controller 背书。

## 2. 固定输入

fixture 由 `tools/make_media_fixtures.py` 离线生成到 `C16_Desktop_Multimedia/build/fixtures`：

- `c16_pulse_mono_48k_s16.wav`：1 秒 mono PCM16。
- `c16_rgb24_pcm_1s.avi`：1 秒 RGB24 视频 + PCM16 音频。
- `c16_empty.media`：空坏输入。
- `c16_truncated.avi`：截断坏输入。

练习的 CMake 在配置时调用生成器。不会下载素材，不读取用户私人媒体。

## 3. 等待方式

播放器是异步对象。正确 probe 需要事件循环，连接这些信号：

- `audioBufferReceived`：统计 audio buffers、frames、format。
- `videoFrameChanged`：统计 video frames 和首帧 size。
- `mediaStatusChanged`：等待 `EndOfMedia` 或 `InvalidMedia`。
- `errorChanged`：记录错误字符串。
- `QTimer`：外部有限等待，避免测试挂死。

只检查文件存在或 `setSource()` 返回不是解码。只数 signal 也不够：空白帧、静音 buffer、错误后端伪造状态都可能让“收到过回调”看起来像成功。本题读取音频样本并按生成器建立 oracle：WAV/AVI 都应解出 48000 frame、48000 Hz、mono；0.1s、0.5s、0.9s 三个窗口应出现大脉冲；1000..1200 sample 的 quiet window 应只包含 440 Hz 小幅 tone。首个视频帧用 `QVideoFrame::toImage()` 取 `(0,0)`、`(20,0)`、`(0,20)`、`(20,20)` 四个像素，与生成器的 RGB 色块公式做容差匹配。bad 变体正是常见错误：它不播放、不等 buffer、不验证坏输入和内容。

## 4. 练习

位置：`C16_Desktop_Multimedia/exercises/L14_qt_playback`。

学生只编辑 `student/solution.hpp`。公开接口：

```cpp
struct DecodeProbe {
    bool saw_audio;
    bool saw_video;
    int audio_buffers;
    qint64 audio_frames;
    int audio_sample_rate;
    int audio_channels;
    double max_abs_sample;
    double pulse1_peak;
    double pulse2_peak;
    double pulse3_peak;
    double quiet_peak;
    qint64 first_audio_start;
    int video_frames;
    QSize first_video_size;
    qint64 first_video_start;
    int video_byte_variation;
    int video_pattern_matches;
    bool eof;
    bool error;
    bool timed_out;
    QString error_string;
};

DecodeProbe probe_media(QString path, int timeout_ms);
```

Part：

1. WAV fixture 必须产生 decoded audio buffer、audio frame、format，并到达 EOF。
2. WAV/AVI 的 decoded audio 必须匹配 48k mono、48000 frame、三个 pulse 窗口和 quiet tone 窗口。
3. AVI fixture 必须产生 decoded audio 和 video frame，首帧尺寸为 64x48，首帧四个采样像素匹配生成器色块，并到达 EOF。
4. 空文件和截断 AVI 必须报告错误。
5. 超时必须设置 `timed_out`，且 checker 一律判 FAIL；不能让测试进程挂起，也不能把超时伪装成坏输入错误。

## 5. 解析

正确实现创建局部 `QMediaPlayer`、`QAudioBufferOutput` 和 `QVideoSink`，把 sink/tap 绑定到 player，再 `setSource()`、`play()`、进入局部 `QEventLoop`。`EndOfMedia` 和 `InvalidMedia` 都会退出；`errorChanged` 也退出；`QTimer` 是最后防线。退出后 `player.stop()`，让对象按栈顺序销毁。

本题只用 Qt 公开头。源码导读可看 [references/standards-and-implementations.md](../references/standards-and-implementations.md) 的 `QMediaPlayer`、FFmpeg playback engine、time controller、audio/video renderer 路径。阅读问题：audio buffer output 与 audio device output 是同一个消费者吗？seek 后后端如何重设时间和队列？
