# L14 Qt playback probe

正文：[L14 QMediaPlayer、QAudioBufferOutput 与 QVideoSink 真实解码](../../chapters/14-qt-playback.md)。

学生只编辑 `student/solution.hpp`。CMake 会调用 `tools/make_media_fixtures.py` 生成固定 WAV/AVI/坏输入到 `C16_Desktop_Multimedia/build/fixtures`，不下载素材、不读取私人媒体。

任务：

1. 用 `QMediaPlayer` + `QAudioBufferOutput` + `QVideoSink` 播放本地文件。
2. 等待 `EndOfMedia`、`InvalidMedia`、`errorChanged` 或 timeout，所有路径有限结束；如果同步完成或同步报错，不能再进入事件循环。
3. 收集 `DecodeProbe` 字段：audio/video 是否出现、audio frames/rate/channels、三个 pulse peak、quiet tone peak、首个 audio/video timestamp、video size、首帧色块匹配数、EOF/error/timed_out。
4. WAV/AVI 必须匹配固定 oracle；空文件和截断 AVI 必须由真实媒体错误拒绝，不能把 timeout 当成功。

有效检查：Reference/good 会真实通过 FFmpeg 后端解码；bad 只看文件存在，因此会被内容 oracle 拒绝。缺扬声器不影响本题，因为解码测试不依赖 `QAudioOutput`。

额外目标：`c16_audio_device` 会使用 `QMediaDevices::defaultAudioOutput()` + `QAudioSink` 向默认输出设备写入约 100ms 静音，验证设备管线能 start、接受数据、`processedUSecs()` 前进、stop 收尾。无默认设备返回 77 SKIP；启动、写入、状态错误或 timeout 都是 FAIL。
