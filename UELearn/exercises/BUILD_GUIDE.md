# UELearn 习题集 构建指南

## 前置

- UE 5.7 源码版已编译, 或 Launcher 版 UE 5.7 已安装
- Windows + Visual Studio 2022 (17.8+), 已安装 "使用 C++ 的游戏开发" workload
- `.NET 6.0 SDK` (UBT 需要)

---

## 打开工程

### 方式 1: 直接用 `UELearn.uproject`

1. 右键 `UELearn.uproject` → **Generate Visual Studio project files**
   - 若右键无此项, 运行: `{EngineRoot}\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe -projectfiles -project="{ABSPATH}\UELearn.uproject" -game -rocket -progress`
2. 生成的 `UELearn.sln` 打开
3. 选择 **Solution Configuration = Development Editor**, **Platform = Win64**
4. 右键 `UELearn` 项目 → **Build**
5. 构建成功后: 右键 `UELearn` → **Set as Startup Project** → F5 启动 Editor (带调试)
6. 或双击 `UELearn.uproject` 直接打开 (无调试)

### 方式 2: 手动关联引擎路径

`UELearn.uproject` 的 `EngineAssociation` 字段为空, 首次打开会提示选择引擎:
- Launcher 版: 选对应版本号
- 源码版: 选 "Binary" 列表里自己编译的引擎实例, 或手动写入引擎绝对路径

---

## 单独编译某一题

每题都是独立 module, UBT 可定向构建:

```
{EngineRoot}\Engine\Build\BatchFiles\Build.bat UELearnEditor Win64 Development -Project="{ABSPATH}\UELearn.uproject" -Module=ExA1_HelloModule
```

---

## 首次 Build 的可能错误

| 错误 | 原因 | 解决 |
|---|---|---|
| `Unable to instantiate module 'ExXN_Topic'` | `.Build.cs` 依赖漏或名字不匹配 module dir | 对照 `UELearn.uproject` Modules 数组, 确认目录名与 `.Build.cs` class 名相同 |
| `unresolved external symbol ... StaticClass` | 头里有 `UCLASS` 但缺 `GENERATED_BODY()` | 补 `GENERATED_BODY()` 到 class 第一行 |
| `fatal error: 'XXX.generated.h' file not found` | UHT 未跑 / include 顺序错 | `CoreMinimal.h` 之后必须 include 本 module 的 `XXX.generated.h` (最后一个 include) |
| `LNK2005 IMPLEMENT_PRIMARY_GAME_MODULE` 重定义 | 多个模块用了 PRIMARY | 仅 `UELearn.cpp` 使用 `IMPLEMENT_PRIMARY_GAME_MODULE`, 其它模块用 `IMPLEMENT_MODULE(FDefaultModuleImpl, <Name>);` |
| Module A 题故意报错 | 教学用, 见题 README 「常见坑」 | 按题指引修复 |

---

## PIE 运行与日志过滤

1. 打开 Editor, 左上 **Play (Alt+P)** 按钮进 PIE
2. **Window → Output Log** 打开日志窗口
3. 日志分类过滤框输入: `LogEx` 可只看习题日志 (每题用 `LogExA1` / `LogExD1` 等独立 category)
4. Module J (网络题) 需 **Play 菜单 → Advanced Settings → Number of Players ≥ 2** 开多客户端 PIE

---

## 清理 / 重建

- 清 build cache: 删 `Binaries/ Intermediate/ DerivedDataCache/ Saved/` 后重开 `UELearn.uproject`
- 某模块反复抽风: `UnrealBuildTool -clean ... -Module=ExXN_Topic`

---

## Capstone 特殊

- **Cap1_UHeightField**: 运行时需创建一个 Content 内的 `UHeightFieldAsset` 资产 (题内 README 附创建步骤), PIE 后触发 `r.UELearn.Cap1.ShowPasses 1` 控制台命令查看 RDG 三 pass
- **Cap2_SourceReading**: 以 markdown 笔记为主, 源码文件在引擎内, 无需 PIE

---

## 建议 IDE 配置

- VS2022: **Tools → Options → Text Editor → C/C++ → View → Auto-expand macros** 开启 (看 `GENERATED_BODY` 展开)
- Rider for Unreal Engine 也可用, 索引更快
