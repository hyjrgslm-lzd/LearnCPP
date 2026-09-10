# U01 ICU Unicode 扩展

本扩展默认不构建。运行前先准备本课隔离 ICU 前缀：

```powershell
.\C05_Data_Representation_Standard_Facilities\exercises\tools\prepare_icu.ps1
```

然后在本目录叶级配置：

```powershell
cmake -S . -B build\icu-debug -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_ENABLE_ICU=ON
cmake --build build\icu-debug --config Debug
ctest --test-dir build\icu-debug -C Debug --output-on-failure
```

Part 1：程序先检查运行时 `u_getVersion == 77.1`、`u_getUnicodeVersion == 16.0`。如果 CMake 找到系统 ICU、错版本 ICU 或 DLL 没复制到测试目录，显式 ON 必须失败。

Part 2：`UnicodeString::fromUTF8` 只在 `c05::validate_utf8` 成功后调用，并使用显式长度。这样嵌入 NUL 会被保留，非法 surrogate UTF-8 会在课程严格层失败，不会被 ICU replacement 语义掩盖。

Part 3：`NormalizationTest.txt` 按 Unicode 16.0 官方数据全量驱动。每条记录验证 NFC/NFD/NFKC/NFKD 的五列公式，不用几条样例冒充完整验证。

Part 4：`GraphemeBreakTest.txt` 使用 root `UBreakIterator` 验证 UAX #29 默认字素边界。程序把 ICU 返回的 UTF-16 code unit offset 映射回 UTF-8 byte offset，证明展示边界、UTF-16 坐标和存储字节坐标是三层不同信息。

Part 5：casefold 与 locale case mapping 分开展示。`foldCase` 可用于派生搜索索引；土耳其语 `I` 的 lower-case 显示要带 locale。两者都不改变 Manifest 的原始 UTF-8 字节身份规则。
