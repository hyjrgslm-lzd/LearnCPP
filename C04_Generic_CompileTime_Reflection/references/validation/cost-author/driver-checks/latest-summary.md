# 作废的 check-only latest-summary

此文件由旧版 `--check-only` driver 误写，不能作为正式结果入口。

新版 `--check-only` 只在各自 `compile-cost-run-*` 目录写 `summary.md` 和 `raw.json`，不会发布 `latest-summary.md`。

正式性能采样完成后，只有 full 模式会在 `C04_Generic_CompileTime_Reflection/references/benchmarks/results/latest-summary.md` 发布当前正式摘要。
