# C01 Coroutine 独立审查记录

结论：APPROVE

审查身份：非作者 reviewer。未修改课程源文件；只写本记录，临时构建限定在 `Coroutine_Study/exercises/build/c01-independent-review/`。

冻结快照绑定：`2026-09-08 15:08:49 +08:00`，`HEAD=c65edb3c2b5c76f386e53fa0afce87c54eb0c363` 加工作区 Coroutine/全局文档修改。

## 本轮只复核的剩余项

前轮已独立验证通过的大矩阵不重复跑：core Release `43/43 passed`、student starter Release 清晰失败、full-windows targeted RPC/bridge Reference `2/2 passed`。本轮只复核前轮剩余 MEDIUM/LOW：

- 临时完成体/坏变体源码是否已从 ignored build 路径迁入随仓库交付材料，并可 fresh rebuild。
- `sync_wait` 旧 `nullopt` 注释是否已修正。
- `quality-report.md` 中的变体验证链接、命令和未验证范围是否与实际一致。

## 当前关键指纹

- `Coroutine_Study/exercises/Capstone5_mini_corolib/include/mini/sync_wait.hpp` `84528980A1ECD6B7580DD6C596B54B55F526A7ACEE6A8C77A673285E997CA7D4`
- `Coroutine_Study/references/quality-report.md` `9DB3DED8636790A9A57956BBE7E1FAC524DA8554931C18CD6556D15C55635E9D`
- `Coroutine_Study/references/validation/c01-supplement/validation_variants/CMakeLists.txt` `272028951F1EC2AA803181832B3E13605EF740A970375F67D521ED215C57B4BA`
- `Coroutine_Study/references/validation/c01-supplement/17-configure-delivered-variants.command.txt` `F41199627CD593A44301688A2F12E344F15CB9AB3851C91CFC840CFF12690271`
- `Coroutine_Study/references/validation/c01-supplement/18-build-delivered-variants.command.txt` `61C35FEE94472A90349E172D4D27AA3E46B803B9295070078DC87DBD6C1E76EE`
- `Coroutine_Study/references/validation/c01-supplement/19-ctest-delivered-variants-good.command.txt` `EF37931F948210974A267340E14A3F82A7AE20B95397FCB51DED029C071968B8`
- `Coroutine_Study/references/validation/c01-supplement/20-ctest-delivered-variants-bad.command.txt` `0A5BCDE6CCB501CCA4CD9584369FCF2543CE9E577AEB9CF1B51A81110336F1E5`

## 复验结果

### 公开变体源

检查路径：`Coroutine_Study/references/validation/c01-supplement/validation_variants/`

结果：

- 源树包含 `CMakeLists.txt`、`good`、`sync_wait_bad_nullopt`、`sync_wait_bad_error_channel`、`scope_bad_noop`、`when_all_bad_error`、`bridge_bad_stopped_value` 的 shadow headers。
- 未发现 `build`、`CMakeFiles`、`Release`、`Debug`、`x64`、`Testing`、`.vcxproj`、`.slnx`、`.exe`、`.obj`、`.pdb`、`.ilk` 等构建产物。
- `CMakeLists.txt` 使用 `COROUTINE_STUDY_ROOT` cache 变量，默认 `../../../..` 相对定位课程根；未写死 `F:/CPPTrain/LearnCPP`。
- 源树内未发现 `mini_ref` alias 或通过 reference 伪装学生实现。

### 公开源 fresh rebuild

独立命令：

```powershell
cmake -S F:\CPPTrain\LearnCPP\Coroutine_Study\references\validation\c01-supplement\validation_variants -B Coroutine_Study/exercises/build/c01-independent-review/freeze-1508-delivered-variants -G "Visual Studio 18 2026" -A x64 -DCOROUTINE_STUDY_ROOT=F:\CPPTrain\LearnCPP\Coroutine_Study
cmake --build Coroutine_Study/exercises/build/c01-independent-review/freeze-1508-delivered-variants --config Release
ctest --test-dir Coroutine_Study/exercises/build/c01-independent-review/freeze-1508-delivered-variants -C Release -R "^good_" --output-on-failure
ctest --test-dir Coroutine_Study/exercises/build/c01-independent-review/freeze-1508-delivered-variants -C Release -R "bad|nullopt|noop|error_channel|stopped_value" --output-on-failure
```

结果：

- Configure/build：通过，MSVC `19.51.36256.0`。
- Good variants：`good_sync_wait`、`good_when_all`、`good_scope`、`good_bridge`，`4/4 passed`，退出码 0。
- Bad variants：`sync_wait_bad_nullopt_sync_wait`、`sync_wait_bad_error_channel_sync_wait`、`scope_bad_noop_scope`、`when_all_bad_error_when_all`、`bridge_bad_stopped_value_bridge`，`5/5 failed`，退出码 8。
- 坏变体失败信息分别命中：未返回 value、未重抛 error、scope 未运行 spawn、when_all 未传播 error、bridge stopped 未抛异常。

### 注释和质量报告

- `sync_wait.hpp:84` 已改为“当前会抛 logic_error 明确失败”，与 `:104-106` 当前实现一致。
- `quality-report.md:70-79` 已指向随 repo 交付的 `references/validation/c01-supplement/validation_variants`，并说明可显式传 `-DCOROUTINE_STUDY_ROOT=<path-to-Coroutine_Study>`。
- `quality-report.md:83-89` 明确列出未验证边界：不声明学生 TODO 已完成、不新增依赖/下载/提交/推送、Linux ASan/io_uring/Folly/Cobalt 未跑。
- 旧 `10..13` 命令文件仍作为历史临时 build 路径证据保留；当前质量报告和追加 `17..20` 证据已使用公开源路径，不影响通过。

## Verdict

APPROVE。前轮两个 HIGH、默认 CTest 污染、Release `assert`、异常失败清晰性、临时完成体/坏变体可复现性、`sync_wait` 注释一致性均已复验通过。

仍需在最终整合报告中保留范围边界：未运行 Linux、ASan、io_uring、Folly、Boost.Cobalt；full-windows 只按本轮范围 targeted 验证 RPC 与 Capstone5 stdexec bridge。

