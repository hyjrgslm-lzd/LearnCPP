# cost-review 静态复验记录

- `git diff -- C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md`：冻结范围无未提交差异。
- `raw.json` quick parse：`CHECK_ONLY_PASS`、`check-only`、detector self-check PASS、type timing samples 0、multi timing samples 0、implicit/explicit stdout equal。
- 独立 parser：从 stored `llvm-readobj --sections` stdout 复算 `.text` section count/raw/virtual，错误数 0。
- 独立 trace parse：从实际 trace JSON 复算 `Total ...` map，和 `raw.json` totals 完全一致。
- 源码契约扫描：type query 包含 `answer/missing/empty` 三个 `static_assert`；multi TU 使用 `std::uint32_t`，每个 `use_N` 有独立 reference 期望；explicit 有 `extern template` 和 provider。
