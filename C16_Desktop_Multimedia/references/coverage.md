# C16 知识覆盖与下游反向检查

本表连接课程承诺、必要先修、正文、真实实现和检查。它是可复用教材导航，不记录某一次构建的通过率。具体执行证据和独立审查版本留在本机忽略目录。

| 下游场景与核心问题 | 必要先修 | 主讲与实现入口 | 验收时必须反查 |
|---|---|---|---|
| 启动应用、分发命令并有限退出 | C01 构建、C02 存活 | [L01 应用](../chapters/01-application-event-loop.md)、[L01 练习](../exercises/L01_application_event/README.md) | QApplication/QCoreApplication、事件到达与回调时机、退出后的责任 |
| 窗口关闭后对象和借用怎样失效 | C02 所有权 | [L02 对象树](../chapters/02-qobject-ownership.md)、[L02 练习](../exercises/L02_qobject_lifetime/README.md) | parent 与栈/heap 的关系、QPointer、DeferredDelete，不把“请求删除”当已释放 |
| worker 的结果在哪个线程执行 | C++ lambda、基本调用 | [L03 元对象](../chapters/03-metaobject-connections.md)、[L03 练习](../exercises/L03_meta_connections/README.md) | moc 的作用、参数/连接/实际发射线程、context 与借用边界 |
| 回调尚未返回却再次修改状态 | L01/L03 | [L04 重入](../chapters/04-dispatch-reentrancy.md)、[L04 练习](../exercises/L04_reentrancy_dispatch/README.md) | queued 不抢占、嵌套循环会重入、状态转换前提和受控反例 |
| 选择新媒体、取消旧分析、关窗收束 | C08 基本同步、L02/L03 | [L05 样章](../chapters/05-worker-cancellation-lifetime.md)、[L05 练习](../exercises/L05_worker_lifecycle/README.md) | 正确同步基线、queued-cancel 复现、原子取消、请求代次、receiver 与线程最终结束 |
| 列表过滤/排序后仍指向原媒体 | C06 容器与失效、L03 | [L06 模型/视图](../chapters/06-model-view.md)、[L06 练习](../exercises/L06_model_view/README.md) | role/index/parent、结构通知、选择映射、持久索引失效、独立业务身份断言 |
| 保存失败、损坏会话与恢复 | C03 错误、C05 路径/JSON | [L07 会话](../chapters/07-application-session.md)、[L07 练习](../exercises/L07_session_lifecycle/README.md) | 先验证后替换、版本/范围、QSaveFile、失败不损旧状态、应用布局与显示条件 |
| 键盘和辅助技术可操作界面 | L01/L06/L07 | [L08 可访问性](../chapters/08-accessibility.md)、[L08 练习](../exercises/L08_accessibility/README.md) | 焦点、名称/角色/状态/动作、自定义视觉控件的语义入口及平台证据边界 |
| 从媒体字节得到样本帧和时间 | C05 字节/整数/时间 | [L09 PCM](../chapters/09-pcm-time.md)、[L09 练习](../exercises/L09_pcm_formats/README.md) | sample 与 frame、声道交织、字节序/格式、尾部、容量和时间换算 |
| 生产快于消费、欠载及流尾 | C08 基本同步、L09 | [L10 有界管线](../chapters/10-bounded-pipeline.md)、[L10 练习](../exercises/L10_bounded_pipeline/README.md) | 明确容量、部分进展、背压、取消/flush/排空；不能无限堆积 |
| 生成真实波形、调整样本与重采样 | L09、C13 数值基础 | [L11 处理](../chapters/11-audio-processing.md)、[L11 练习](../exercises/L11_audio_processing/README.md) | RMS/peak 与独立数值期望、混音/裁剪、min/max、插值及抗混叠边界 |
| 从视频帧读取图像而不越界 | C02 借用、C05 表示 | [L12 视频帧](../chapters/12-video-frames.md)、[L12 练习](../exercises/L12_video_frames/README.md) | 真实 map/unmap、plane/stride、色彩范围、共享资源及失效时刻 |
| 暂停/速率/seek 后仍按正确时间呈现 | L09、C05 单调时间 | [L13 时钟](../chapters/13-media-clock.md)、[L13 练习](../exercises/L13_media_clock/README.md) | PTS/DTS/时基、先结算再改锚点、过期帧、等待/显示/丢弃及模型与真实后端区别 |
| 实际解码音视频并处理错误 | L02/L03/L09/L12/L13 | [L14 Qt 播放](../chapters/14-qt-playback.md)、[L14 练习](../exercises/L14_qt_playback/README.md) | 固定 WAV/AVI 的真实内容、格式、时间戳、EOF 与失败；缺设备不掩盖解码失败 |
| 从卡顿现象定位通知成本 | L03/L05、C13 测量 | [L15 响应性](../chapters/15-responsiveness.md)、[L15 练习](../exercises/L15_responsiveness/README.md) | 改动前基线、阶段与回调计数、同契约合并、原实验复验、所有独立样本与局限 |
| 把应用、线程和媒体接成真实工具 | 上述各单元按所用功能 | [P1 工作台](../chapters/16-media-workbench.md)、[P1 练习](../exercises/P1_media_workbench/README.md) | 文件→模型→选择→分析/播放→显示→标记→保存恢复→关闭，实际按钮不能仅由直接 controller 调用替代 |
| 第二种界面消费同一业务状态 | L03/L06/P1 | [U01 Quick](../chapters/17-quick-bridge.md)、[U01 练习](../exercises/U01_quick_bridge/README.md) | 真正可见窗口、required properties、role/notify、绑定与对象归属、实际 QML 消费者 |

## 检查覆盖与教学完成是两件事

Reference 和 good 通过证明指定输入与不变量符合检查。bad 被拒绝证明检查器能观察对应的错误，不证明所有错误都会被检出。Student 初始失败证明它仍需实现；观察型入口成功也不证明读者已经完成解释和扩展 Part。

每个项目使用点要反向回到本表：若 P1 的实际代码出现未讲过的所有权、线程切换、时基、媒体格式或模型规则，就回补主讲与验证。源码阅读同样需要明确入口、关键状态和退出路径；固定版本入口见 [规范与源码索引](standards-and-implementations.md)。

本课新建这些主题，没有迁移或删除其他课程的知识。通用语言/容器/同步/测量仍由其主课负责；Qt 框架对象模型、桌面生命周期与媒体接口在本课展开。Qt 元对象与 C++ 语言反射的差异通过 C04 桥接，不以命名相似推断机制等价。
