# C16 规范、实现与源码阅读

本课固定 Qt **6.9.2**，核对日期为 2026-09-14。课程使用 C++23 普通头文件，不依赖 C++ Modules、未来标准库或 Qt 私有头。Qt 的元对象、信号、线程归属和媒体后端是框架协议，不能当作 ISO C++ 对象或线程规则的替代物。

## 怎样判断一条结论

| 信息 | 能支持什么 | 不能支持什么 |
|---|---|---|
| C++ 语言规则与前置课推导 | 存活、数据竞争、捕获、值类型及异常语义 | 某个 Qt 对象允许在哪个线程调用 |
| Qt 6.9 系列公开文档 | 公开 API 契约、支持条件与使用方式 | 某个机器上插件一定能加载 |
| `v6.9.2` 源码 | 该版本具体的状态、锁、事件与退出路径 | 跨版本永久保持相同实现 |
| 本课真实 Qt 检查 | 当前二进制与环境下的可观察行为 | 所有可能交错、平台或设备的完整证明 |
| 受控媒体时钟/队列实验 | 给定契约下的决策、容量和计算正确性 | Qt 内部采用了同一算法，或物理扬声器延迟已测出 |

Qt 归档站点可能展示 6.9 系列最后一个补丁版本。若归档说明与本课安装版本有差异，应以 `v6.9.2` 的对应声明、实现和真实实例化复核，而不是直接把更新后的接口写进课程。安装目录的版本元数据也不等于完整源码与官方 tag 的逐文件一致性证明。

## 公开入口

- [Qt 对象模型](https://doc.qt.io/qt-6.9/object.html)：对象树、属性、信号及运行时模型。
- [线程与 QObject](https://doc.qt.io/archives/qt-6.9/threads-qobject.html)：对象归属、事件循环、连接执行位置与删除。
- [模型/视图编程](https://doc.qt.io/qt-6.9/model-view-programming.html)：model、view、delegate、索引与通知。
- [Qt 可访问性](https://doc.qt.io/qt-6.9/accessible.html)：语义接口、平台桥接及自定义控件责任。
- [QAudioBufferOutput](https://doc.qt.io/archives/qt-6.9/qaudiobufferoutput.html)：Qt 6.8 起提供的解码音频观察接口，只支持 FFmpeg 后端。
- [QVideoFrame](https://doc.qt.io/archives/qt-6.9/qvideoframe.html)：映射、平面、stride、时间戳与共享缓冲。
- [Qt Multimedia](https://doc.qt.io/archives/qt-6.9/qtmultimedia-index.html)：播放、设备及后端边界。
- [C++ 模型与 Quick](https://doc.qt.io/archives/qt-6.9/qtquick-modelviewsdata-cppmodels.html)：模型角色、变更通知与 QML 消费者。

这些链接是出处和继续阅读入口。完成本课练习所必需的因果推导仍写在各章，不要求读者靠外部文章补齐答案。

## 阅读链一：调用怎样成为另一个线程上的事件

固定源码：`qtbase/src/corelib/kernel/qobject.cpp` 与 `qcoreapplication.cpp`，tag `v6.9.2`。在 [Qt 源码树](https://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/kernel/qobject.cpp?h=v6.9.2) 中搜索函数名比记住滚动行号可靠。

先提出问题：连接存在时，`emit` 是否保证槽立即执行？连接发起者所属线程、实际发射线程和 receiver affinity，哪一个决定执行位置？然后沿下列状态读：

1. 在连接建立路径找到 sender/receiver、连接类型和可调用对象怎样保存。连接成功只建立关系，没有把所有未来参数预先保存。
2. 在激活连接的路径定位 `queued_activate`。关注参数的元类型、复制及事件对象如何形成；不能把栈上借用指针伪装成被复制的数据。
3. 进入 `QCoreApplication::postEvent`。观察事件进入哪个 thread data 的队列、队列如何保护、何时唤醒分发器。一个事件已经入队不表示目标线程已经获得 CPU。
4. 进入 `sendPostedEvents` 与接收者分发。记录事件执行、对象析构和重复进入事件处理之间的关系。
5. 对照 `QObject::deleteLater`。DeferredDelete 依赖相应循环及退出规则，不能把“调用了 deleteLater”写成“这一行之后已经释放”。

回到 L03—L05：分别观察 direct/queued 的顺序、worker 卡在受控 gate 时 queued cancel 的位置、receiver 销毁与过期请求的区别。**对象还活着**和**结果仍属于当前请求**是两个独立条件。源码阅读应画出一条“发射→复制/投递→目标循环→调用/丢弃→资源释放”的时序图，并解释每个失败出口。

## 阅读链二：视图为何必须知道结构改变的边界

固定源码：`qtbase/src/corelib/itemmodels/qabstractitemmodel.cpp`，tag `v6.9.2`。从 `beginInsertRows`、`endInsertRows`、`beginResetModel` 进入，而不是从一个已经画好的表格倒猜规则。

问题是：修改 `std::vector` 或 `QList` 已经成功，为什么界面还可能显示旧内容，甚至让选择指向另一项？容器只维护它自己的存储，不会自动通知持有 `QModelIndex` 的消费者。

沿源码记录：修改前的结构通知、容器实际改变、持久索引更新和修改完成通知。把插入、删除、移动与 reset 分开；reset 的语义更强，不能为了少写代码就假装它保留了所有选择与索引。普通索引、持久索引以及媒体稳定 ID 各自解决不同问题。

在 L06 的真实 model 上连接视图和代理，插删后核对数据、角色和所选媒体身份。`QAbstractItemModelTester` 能发现许多模型协议错误，但不会替你定义“用户本来选的是哪一个文件”。因此还需要独立的身份和内容断言；仅保留 Warning 日志而测试退出 0 不能算检查有效。

## 阅读链三：Qt 播放器怎样映射时间与输出

固定源码来自 `qtmultimedia`，tag `v6.9.2`：

- `src/multimedia/playback/qmediaplayer.cpp`：公开 `setSource`、状态与音频观察接口。
- `src/plugins/multimedia/ffmpeg/qffmpegplaybackengine.cpp`：播放器、解码与渲染对象的连接及重设。
- `src/plugins/multimedia/ffmpeg/playbackengine/qffmpegtimecontroller.cpp`：时间锚点、速率、暂停与软同步。
- 同目录 `qffmpegaudiorenderer.cpp`、`qffmpegvideorenderer.cpp`：帧交付、输出等待与结束收束。

这些后端类属于实现细节。课堂应用只链接公开 Qt 目标，不包含 `_p.h`，也不直接实例化私有播放器类型。

先在 `QMediaPlayer` 的 audio buffer 输出说明中查三个边界：buffer 是已经解码的样本；其格式可能来自指定输出或源流；buffer 到达与扬声器真正播放之间还可能存在缓冲延迟。速率改变也不意味着回调样本已经按同一比例重新采样。因此 `audioBufferReceived` 的主机时间不能直接标成“听到声音的时间”。

接着读 `TimeController`：`sync` 建立媒体位置与单调时钟锚点；`setPlaybackRate` 在改变斜率前先结算旧映射；`setPaused` 在冻结前也先结算当前位置。若直接改变速率而保留旧锚点，就可能让媒体位置突然跳跃。L13 用可控时钟验证这一数学关系，源码对照则解释 Qt 的特定实现还增加了什么。

最后沿音频 renderer 的 `renderInternal`、`pushFrameToOutput` 和 `pushFrameToBufferOutput` 跟踪同一帧：音频设备可能只能接受一部分字节，未提交部分需要保留；观察 buffer 与实际输出是不同消费者；到达流尾与设备队列已经排空也不是同一个事件。对照视频 renderer 的时间等待与丢弃路径，记录 seek 后哪些队列/锚点被重设。不要从一个 `play()` 调用省略到“Qt 自动同步好了”。

源码导读作业：选择播放、暂停、seek、结束中的一条路径，写出入口、关键对象、状态改变、输出/失败事件和最终资源归属，再指向本课可运行的对应检查。答案必须说明哪些部分由真实 Qt 运行观察支持，哪些只是阅读该版本实现所得。

## 依赖与部署记录

课程只提交自有源码、构建/检查工具和确定性输入规格。Qt 安装、插件和 FFmpeg 动态库留在机器或被忽略的部署输出；不把依赖二进制混入课程源码。

需要记录依赖来源时，读取实际 Qt 版本元数据、使用到的模块、后端 DLL，以及上游源码的 `LICENSES` 和 SPDX 标识。源码归档没有 Git 元数据时如实标注，不编造 commit SHA。教材给出固定 tag 导航；某次文件指纹与实际加载结果保存在本机证据目录。
