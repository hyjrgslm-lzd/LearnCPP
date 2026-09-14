# L13 媒体时钟、PTS/DTS、暂停、速率与 seek generation

音视频同步不是“收到帧就显示”。每帧带有媒体时间戳，播放器还持有一个单调主机时钟到媒体时间的映射。播放、暂停、倍速和 seek 都是在改这条映射。本章用可注入 `ManualClock` 建立一个小模型，证明自己的契约；它不声称 Qt 内部调度完全相同。

## 1. PTS 与 DTS

DTS 是解码顺序时间戳，PTS 是呈现时间戳。有 B-frame 的视频可能先解码后显示；UI 同步关心的是“什么时候呈现”，所以本章 frame decision 使用 PTS。练习不构造压缩 GOP，只保留概念边界：输入 frame 结构只含 `pts_us` 和 `generation`。

单位必须先固定。Qt 的很多媒体 API 对外用毫秒，`QVideoFrame::startTime()` 和 `QAudioBuffer::startTime()` 使用微秒，FFmpeg 内部常以 stream time base 表示，需要通过分子/分母换算。课程模型统一用微秒 `int64`，避免在一个函数里混用 ms/us。实际接 Qt 信号时，第一步应把来源单位转换成同一时基，再进入同步策略。

## 2. 单调锚点

播放时钟用两个锚点表示：

```text
anchor_wall_us: 单调时钟时刻
anchor_media_us: 当时对应的媒体位置
rate: 斜率
position = anchor_media_us + (now - anchor_wall_us) * rate
```

这个公式仍有输入域。练习把所有时间统一为微秒 `long long`，但只接受可表示、非负的媒体位置；`ManualClock::advance_us()` 只接受非负增量，且加法不能溢出。`rate` 必须是有限正数，并限制在 `0 < rate <= 64`。`start()`、`seek()`、`set_rate()` 会先验证新输入和旧锚点能否结算，失败时不改写已有锚点。`classify()` 比较极端 PTS 时不能直接用有符号整数相减，否则 `LLONG_MIN - LLONG_MAX` 会溢出；应改用扩大精度比较，或用不溢出的分支比较。

暂停时先结算当前位置，再冻结。恢复时重设 wall anchor。改变速率前也必须先结算旧速率下的位置，否则位置会跳。seek 则直接设置媒体位置并增加 generation。

主时钟选择是策略，不是语法。常见选择：

- 音频主时钟：适合有真实音频设备的播放，因为人耳对音频抖动敏感，视频可丢帧追音频。
- 视频主时钟：适合无音频或逐帧预览。
- 外部单调时钟：适合教学、录制回放或测试，可完全复现。

物理设备会漂移。声卡实际采样率可能略偏离标称 48000，显示刷新也有抖动。生产播放器会用反馈、缓冲水位、丢帧/等待策略持续修正。本章不做漂移控制，只要求你把“媒体位置 = 锚点 + 单调时间差 * rate”的基础关系写对；漂移属于后续响应性和设备验证。

### 反例 1：pause 不结算

假设当前位置 501000us，暂停 500000us。如果 `pause()` 只设一个布尔值，但没有把 `anchor_media_us` 改成 501000，那么 `position_us()` 可能继续用旧锚点计算，暂停期间仍增长。检查器先运行 0.5 秒、暂停、再推进 0.5 秒，要求位置不变。

### 反例 2：rate 不结算

当前位置 501000us 时改成 2x。若只改 `rate`，仍用最初 wall anchor，下一次计算会把过去 0.5 秒也按 2x 重算，位置瞬间跳到约 1001000us。正确做法是：先用旧 rate 结算当前位置，再把当前 wall/media 作为新锚点，之后的时间差才按 2x。

## 3. generation

seek 后旧队列里的 frame 可能还会晚到。仅比较 PTS 不够，因为旧视频的 PTS 可能刚好接近新位置。generation 把“属于哪次 seek 之后的队列”编码进 frame。旧 generation 一律 drop。

这和 L05 的请求 generation 是同一类思想：对象还活着、时间戳接近，都不代表数据属于当前请求。

seek 反例：旧队列里有一帧 PTS=1s，用户 seek 到新媒体位置 1s。只看 PTS 会显示旧帧；只看对象存活也会显示旧帧。generation 把“这是第几次 seek 后生产的帧”写进状态，旧 generation 直接 Drop。检查器在 `seek(1000000)` 后拿旧 generation 的 `pts=1000000` 检查，必须丢弃。

## 4. 显示、等待、丢帧

本题策略：

- frame generation 旧：Drop。
- PTS 比当前媒体位置超前超过 30ms：Wait。
- PTS 比当前位置落后超过 30ms：Drop。
- 否则 Display。

这只是教学策略。真实播放器可能结合音频主时钟、vsync、解码队列、渲染线程和设备延迟。L14 使用 `QMediaPlayer` 真实播放/解码，但不把本章模型伪装成 Qt 后端内部算法。

30ms 不是标准常数，而是本题契约。它约等于 33.3ms 的 30fps 一帧，用来制造清晰边界：+90ms 必须 Wait，-90ms 必须 Drop，+10ms 可以 Display。真实应用应按目标帧率、渲染延迟、音频缓冲和用户操作场景设阈值，并记录证据。

## 5. 练习

位置：`C16_Desktop_Multimedia/exercises/L13_media_clock`。

Part：

1. `start()` 后随 `ManualClock` 前进更新 media position。
2. `pause()` 冻结；`resume()` 后继续。
3. `set_rate()` 先结算旧位置，再改变斜率。
4. `seek()` 设置新位置并递增 generation。
5. `classify()` 对 frame 做 wait/display/drop 决策。

`bad` 变体不结算 pause/rate，不递增 generation，检查器会拒绝。

## 6. 解析

正确实现没有线程。它只处理纯数学状态，这让错误可复现：给定同一组 `advance_us()` 调用，结果固定。将来接真实 Qt 时，单调时钟来源可以换成 `QElapsedTimer` 或后端 time controller，但仍要保留“结算旧锚点再改状态”的原则。

逐 Part 对应关系：

1. `start(1000)` 建立 `wall0=now`、`media0=1000`、`paused=false`。推进 500000us 后位置应是 501000us。
2. `pause()` 调 `position_us()` 结算，再冻结。暂停期间 `ManualClock` 继续走，但 position 不走。
3. `set_rate(2.0)` 先结算当前 media，再重设 wall anchor。之后 100000us 主时钟增量映射成 200000us 媒体增量。
4. `seek(2000000)` 设置 media anchor 并 `++generation`。返回值让调用方给后续 frame 打标签。
5. `classify()` 先查 generation，再用 `frame.pts_us - position_us()` 与阈值比较。顺序不能反过来；旧 generation 即使时间接近也必须 Drop。
6. 边界检查拒绝负 `advance_us()`、负 `start/seek`、非有限或过大的 `rate`、会溢出的 clock/media 位置；这些失败不能留下半更新状态。

源码阅读入口见 `qffmpegtimecontroller.cpp`。阅读时找 `setPaused`、`setPlaybackRate`、`sync` 一类函数，观察它如何保存当前媒体位置与主机时钟。私有实现只作为 Qt 6.9.2 的对照，不进入练习 include。
