# C05 ICU 77.1 准备记录

ICU 扩展默认关闭；只有显式启用 `DATA_STUDY_ENABLE_ICU` 或手动运行 U01 相关验证时才依赖本页准备的本地前缀。

## 固定版本

- 上游：`https://github.com/unicode-org/icu.git`
- 固定提交：`457157a92aa053e632cc7fcfd0e12f8a943b2d11`
- ICU：77.1
- Unicode：16.0
- 本地前缀：`C05_Data_Representation_Standard_Facilities/exercises/build/_deps/icu-77.1`

## 准备命令

从仓库根目录运行：

```powershell
.\C05_Data_Representation_Standard_Facilities\exercises\tools\prepare_icu.ps1
```

脚本只写入本课隔离前缀和 `references/validation/icu-prepare-*` 证据目录；不会全局安装 ICU、修改 `PATH`、改注册表、调用 vcpkg/conan、提交或推送。

## 构建策略

脚本复用 C02 `record_process.py` 记录所有外部命令，超时和进程树清理由 C01 `process_runner.py` 负责。源码获取使用 sparse checkout，只取 `icu4c`，避免 ICU4J 测试资源在 Windows 上触发长路径问题。

脚本按官方 ICU4C Windows 命令行构建方式调用：

```text
source\allinone\allinone.sln
MSBuild /p:Configuration=Debug|Release /p:Platform=x64 /p:PlatformToolset=v145 /p:SkipUWP=true
```

使用 `/m:1` 降低 MSVC PDB 并发锁风险。构建输出复制到：

- `include`
- `lib64`
- `bin64`

## 验证策略

脚本生成一个最小 CMake 探针项目，使用原生 `FindICU`：

```cmake
find_package(ICU 77.1 EXACT REQUIRED COMPONENTS uc i18n data)
```

探针会分别 Debug/Release 构建并运行，运行时把本课 `bin64` 临时加到子进程 `PATH`。程序断言 `u_getVersion` 为 77.1、`u_getUnicodeVersion` 为 16.0，并执行一个 NFC 归一化烟测。

最终证据在最新 `references/validation/icu-prepare-*/summary.json`，其中记录提交、命令 exit、输出文件、库/DLL SHA256、license SHA256、Debug/Release 探针结果。`summary.json` 为 `PASS` 只表示 ICU 77.1 依赖准备完成，不表示完整 C05 ICU 课程验收完成。
