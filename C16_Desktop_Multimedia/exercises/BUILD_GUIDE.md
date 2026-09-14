# C16 构建与运行

## 环境契约

最低 CMake 3.24、Python 3.10、C++23。完整课程固定 **Qt 6.9.2**，需要 Core、Gui、Widgets、Multimedia、MultimediaWidgets、Qml、Quick、QuickControls2、Test。使用共享 Qt 安装，不编译 Qt 本身，不自动下载或更换版本。

Windows 主验收使用 MSVC x64。`msvc2022_64` Qt 包需要兼容的 MSVC ABI，不能与 MinGW 对象混合链接。这里给出的安装路径只是命令示例，项目本身不写死本机目录。源码导读使用相同版本的 Qt 源码树，但构建课程不需要整份 Qt 源码。

进入 Visual Studio 的 x64 Native Tools Command Prompt，再运行配置和构建。PowerShell 中仅能找到 `cmake.exe` 并不表示 MSVC 的 INCLUDE/LIB 已配置。本机示例使用 NMake Makefiles，并设置 `CMAKE_DEPENDS_USE_COMPILER=FALSE`，让 CMake 扫描头文件依赖，避开中文 MSVC 的 `/showIncludes` 前缀编码问题。不要通过改变系统语言或安装组件来制造通过；其他机器可在确认头文件依赖解析正确后使用 Ninja。

下面命令在 `C16_Desktop_Multimedia` 目录执行。把 Qt 前缀换成自己的 6.9.2 安装；Ninja 也应使用已知可运行版本。

## 默认只构建

下面只配置、编译和链接，不启动课程程序或 CTest。构建过程中仍会调用编译器、Qt moc/rcc 和离线夹具生成工具。

```powershell
cmake -S exercises -B build/release -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64"
cmake --build build/release

cmake -S exercises -B build/debug -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64"
cmake --build build/debug
```

选择 Ninja 时可增加 `-DCMAKE_MAKE_PROGRAM="D:/Qt/6.9.2/Tools/Ninja/ninja.exe"`，并使用全新构建目录，不能在旧缓存中切换生成器。`Configuring done` 与 `Generating done` 只证明工程生成，必须继续实际编译链接。缺 Vulkan 头文件等可选能力的提示，不等于本课 Widgets/软件 Quick/CPU 媒体路径不可用；依赖解析或目标构建失败则必须处理。

完整配置缺必需 Qt 模块时明确失败。没有 Qt 的纯算法观察不能冒充整门桌面课程通过。CMake 不会自动降级到其他 Qt 版本或更换媒体后端。

## Student 与单题入口

```powershell
cmake -S exercises -B build/student -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64" -DC16_BUILD_REFERENCE=OFF
cmake --build build/student

cmake -S exercises/L05_worker_lifecycle -B build/l05 -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64"
cmake --build build/l05
```

默认编译 Student，但不自动执行未完成作业。加 `-DC16_TEST_STUDENTS=ON` 才把它们登记到 CTest；初始 Student 应真实失败，不能改成 SKIP 或调用 Reference 让报告变绿。`C16_BUILD_REFERENCE=OFF` 关闭参考答案和 good/bad 控制组，保留学生入口与独立观察入口。

四种实现共用输入和检查器，检查器实际调用所选实现。`reference` 是完整答案；`good` 是独立正确控制；`bad` 是行为错误控制。bad 只有以指定检查诊断、正常非零检查退出码结束才算被正确拒绝，崩溃、程序缺失和超时都是真失败。Release 检查不依赖会被 `NDEBUG` 关闭的 `assert`。

## 可选：运行检查与应用

运行验证与编译是独立步骤，仅在需要验证运行行为时显式执行：

```powershell
python tools/run_check.py --timeout 180 -- ctest --test-dir build/release --output-on-failure
python tools/run_check.py --timeout 180 -- ctest --test-dir build/debug --output-on-failure
```

编译成功只证明所选工具链能够生成可执行文件，不证明运行时正确性、媒体后端、设备或 GUI 交互已经通过验证。

## Qt 运行时与媒体输入

从命令行直接启动程序前，把所用 Qt 的 `bin` 目录加入当前进程 PATH。CTest 自动设置对应路径，并为自动检查设置 `QT_QPA_PLATFORM=offscreen`、`QT_QUICK_BACKEND=software`、`QT_MEDIA_BACKEND=ffmpeg`。这些设置仅作用于测试子进程，不改变机器配置。

`CMAKE_DEPENDS_USE_COMPILER=FALSE` 是 [CMake 的 Makefile 生成器选项](https://cmake.org/cmake/help/latest/variable/CMAKE_DEPENDS_USE_COMPILER.html)，不是禁用依赖更新；生成的 `depend.make` 仍应包含所选 `solution.hpp` 和公共检查头。若换用 Ninja，不沿用这一生成器专属回退，须先确认头文件改动会触发重编译。

```powershell
python tools/make_media_fixtures.py --output build/fixtures
```

夹具由确定性输入生成，包含 PCM WAV 与短 RGB/PCM AVI；不读取私人音视频、不联网获取样本。生成物留在 `build/`。格式、预期样本和生成边界见工具本身及媒体章节。真实播放与解码入口会使用这些夹具；不把生成器自己的检查当成 Qt 已解码的证据。

`QAudioBufferOutput` 需要 FFmpeg 媒体后端。Qt 安装包中存在插件 DLL 不证明它一定能加载，必须继续运行真实解码检查。无音频输出设备不应使解码观察被悄悄跳过；有声播放单独验证。

## 超时、证据与平台边界

本课验收统一经 `tools/run_check.py` 启动整体 CTest；每项检查再经同一工具提供进程外超时，CTest 另留清理余量。测试子进程的 TEMP/TMP/TMPDIR 指向对应构建目录的 `test-tmp`，各次 QTemporaryDir 仍独立生成随机子目录，避免依赖不同终端的系统临时目录权限。超时、无法启动和无法确认清理均算失败，不以内部 stop 标志替代。测试只能清理自己启动的进程。

先运行短正确性检查，再按响应性章节执行独立采样。采样必须保持源码、可执行文件、输入和计时口径固定；原始失败与无收益结果保留。没有阶段定位证据时，数字只能描述现象，不能证明某种瓶颈。

无窗口 Qt 检查、真实媒体解码、原生窗口和键盘、辅助技术接口、设备输出及部署目录运行各有独立边界。offscreen 通过不证明窗口正确显示；音频缓冲到达不证明实际扬声器音画同步；QAccessible 属性存在也不能证明所有屏幕阅读器都已验证。

本机日志、测量样本、审查记录与交付指纹保存在 `build/`。教材保存复现方法和判断规则，不链接私人运行记录。其他平台和未具备设备明确列为未验证，不自动复制 Windows 的结论。
