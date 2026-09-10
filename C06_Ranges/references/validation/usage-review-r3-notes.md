# usage-review-r3 validation notes

- 日期：2026-09-10
- 绑定 HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`
- 审查报告：`C06_Ranges/references/reviews/usage-review-r3.md`
- 作者验证目录：`C06_Ranges/validation/usage-review-r3-build-20260910/`

## 范围

仅复验 usage r2 遗留的 2 个 BLOCK：

- `C06_Ranges/exercises/C1_1_join/README.md`
- `C06_Ranges/exercises/C2_3_ranges_to/main.cpp`

辅助核对：

- `C06_Ranges/exercises/C1_1_join/main.cpp`
- 作者 r3 `summary.json` 与 `source-hashes.json`

未修改作者源码，未覆盖旧 `usage-review.md`、`usage-review-r2.md`、旧 build 失败记录或作者原始记录。

## 独立验证命令摘要

对 `C1_1_join` 和 `C2_3_ranges_to` 分别使用独立 build dir：

```powershell
cmake -S C06_Ranges/exercises/<unit> -B C06_Ranges/references/validation/usage-review-r3-build-20260910/<unit>
cmake --build <build-dir> --config Release
ctest --test-dir <build-dir> -C Release --output-on-failure
cmake --build <build-dir> --config Debug
ctest --test-dir <build-dir> -C Debug --output-on-failure
```

结果：

- `C1_1_join` Release ctest：pass
- `C1_1_join` Debug ctest：pass
- `C2_3_ranges_to` Release ctest：pass
- `C2_3_ranges_to` Debug ctest：pass
- 合计：4/4 ctest pass，0 failure

## 原始证据

- 独立复验 summary：`C06_Ranges/references/validation/usage-review-r3-build-20260910/summary-f261bea5.json`
- 独立复验完整结果：`C06_Ranges/references/validation/usage-review-r3-build-20260910/results-f261bea5.json`
- 本轮文件哈希：`C06_Ranges/references/validation/usage-review-r3-filehashes-f261bea5.json`
- 静态扫描简表：`C06_Ranges/references/validation/usage-review-r3-static-scan-f261bea5.txt`
- 静态扫描分项：`C06_Ranges/references/validation/usage-review-r3-static-scan-detail-f261bea5.txt`

## 内容核对

- `C06_Ranges/exercises/C1_1_join/README.md:23-28` 已把 stored join common 与 non-common 例子拆开。
- `C06_Ranges/exercises/C1_1_join/README.md:56-58` 已把 `join_view` common 性改成条件化表述。
- `C06_Ranges/exercises/C2_3_ranges_to/main.cpp:42-46` 已使用 `part | std::ranges::to<std::string>()`，独立编译通过。
- 分项静态扫描未发现旧 `not common_range`、旧 `std::string(part)`、`TODO`、`SKIP`、`#if`、`catch (` 或明显凭据字符串。

