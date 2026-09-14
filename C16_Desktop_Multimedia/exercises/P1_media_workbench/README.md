# P1 MediaWorkbench

对应正文：`../../chapters/16-media-workbench.md`。

## 你要完成什么

学生练习只编辑 `student/solution.hpp` 里的 `p1::ProjectState`。它覆盖 MediaWorkbench 的状态核心：

- 本地媒体路径生成稳定 ID。
- 重复添加同一路径不重复建 row。
- 过滤后按可见 row 选择真实 item。
- 时间标记绑定当前 selected ID。
- session 用 version/items/selected/filter/markers 保存和恢复。

完整 Widgets app 在 `app/workbench.hpp/.cpp`，target 是 `c16_workbench`。它独立 smoke，不因为 `ProjectState` 通过就宣称 UI 通过。

## 已提供

- `student/solution.hpp`：安全占位，默认失败。
- `reference/solution.hpp`：完整参考实现。
- `good/solution.hpp`：独立正确实现，不引用 Reference。
- `bad/solution.hpp`：真实错误实现，重复行、忽略过滤、假保存。
- `checks.cpp`：实际调用所选 `solution.hpp`。
- `app/`：Qt Widgets MediaWorkbench，复用 `MediaListModel`、`SessionController`、`AnalysisWorker`。

## 检查什么

四变体检查验证状态作业。`c16_workbench_smoke` 额外验证：

- QTest 驱动过滤框、列表、play/mark/save/restore/seek 控件。
- WAV/AVI 夹具和临时 oracle 输入真实解析出 duration、peak、waveform；覆盖 24 kHz、额外 RIFF chunk、多 `01wb` 分块、坏 fmt、奇数 PCM 尾部。
- `QMediaPlayer + QAudioOutput + QVideoWidget` 收到音频 buffer 和视频 frame。
- corrupt/unknown-version/missing-path/bad-selected/bad-marker/oversized restore 失败且旧状态不变，空 session 可 round-trip。
- empty/truncated media 显示错误。
- snapshot 包含分析完成后的非空波形。

## 命令

```powershell
cmake -S C16_Desktop_Multimedia/exercises/P1_media_workbench -B C16_Desktop_Multimedia/build/p1 -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64"
cmake --build C16_Desktop_Multimedia/build/p1
ctest --test-dir C16_Desktop_Multimedia/build/p1 --output-on-failure
```

普通 app：

```powershell
C16_Desktop_Multimedia/build/p1/c16_workbench.exe
```

自动 smoke：

```powershell
C16_Desktop_Multimedia/build/p1/c16_workbench.exe --self-check --fixtures C16_Desktop_Multimedia/build/fixtures --snapshot C16_Desktop_Multimedia/build/p1/workbench.png
C16_Desktop_Multimedia/build/p1/c16_workbench.exe --fixtures C16_Desktop_Multimedia/build/fixtures --snapshot C16_Desktop_Multimedia/build/p1/workbench.png
```
