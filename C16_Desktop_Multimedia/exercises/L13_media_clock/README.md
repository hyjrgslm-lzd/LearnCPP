# L13 media clock

正文：[L13 媒体时钟、PTS/DTS、暂停、速率与 seek generation](../../chapters/13-media-clock.md)。

学生只编辑 `student/solution.hpp`。本题使用可注入 `ManualClock`，单位统一为微秒。

任务：

1. 用 `anchor_wall_us + anchor_media_us + rate` 建立媒体时间映射。
2. `pause()` 和 `set_rate()` 必须先结算旧位置，再改变状态。
3. `seek()` 设置新媒体位置并递增 generation。
4. `classify()` 先拒绝旧 generation，再按 `pts - position` 与 30ms 阈值返回 Wait/Display/Drop。
5. 拒绝负 `advance_us()`、负 `start/seek`、非有限或 `>64` 的 rate，以及会溢出的 clock/media 位置；失败不能半更新锚点。

有效检查：pause 冻结、2x rate、seek generation、future wait、late drop、旧 generation drop、负增量、负 seek/start、极大 rate、溢出和极端 PTS 都会被检查。bad 会因不结算、generation 不变、阈值错误、边界不安全被拒绝。
