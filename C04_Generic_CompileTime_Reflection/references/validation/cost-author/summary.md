# B01 compile cost 作者状态

2026-09-09，本轮已完成 B01 叶级源码、正文、正式 driver 和默认正确性自检。根据 leader 反馈，正式性能采样暂停到全部 builder 自检结束后再跑；本轮只做 driver 与 baseline 缺陷修复、最小 correctness/tool-parser 验证。

## 已完成

- `exercises/B01_compile_cost/CMakeLists.txt`：叶级 CMake，默认只注册 `B01_compile_cost_contracts` 观察程序。
- `exercises/B01_compile_cost/README.md`：说明默认构建与显式正式 benchmark 入口。
- `exercises/B01_compile_cost/observations/contracts.cpp`：有限正确性观察，覆盖类型查询契约和 `extern template` 单定义形态。
- `references/benchmarks/cost_driver.py`：Python 3.10 driver，生成递归/折叠、隐式/显式实例化源码；分开 correctness、trace、section/symbol 和 timing；一次预热、五次独立进程样本；固定随机种子 `40412025`；按进程名/PID/CPU 只读检测构建重叠。
- `chapters/12-compile-cost.md`：正文按成本模型、归因假设、测量入口、解释边界和代价组织。

## 本轮修复

- 多 TU baseline 改为 `std::uint32_t` 定义良好的模 2^32 算术，不再使用 signed 溢出或可能负值左移。
- 多 TU `main` 对每个 `use_N` 使用独立 reference 计算核对，并打印四个实际输出；`--check-only` 已确认 implicit 与 explicit stdout 完全一致。
- section parser 改为解析实际 COFF `llvm-readobj --sections` 输出，累加所有 `.text` / `.text$*`，分别记录 `text_raw_total`、`text_virtual_total`、`text_section_count` 和完整原始工具输出；解析失败为 FAIL。
- MSBuild node-reuse 改为前后 PID/CPU 快照：`cl/cmake/link/ninja/clang-cl/lld-link` 存在直接视为 active；`MSBuild.exe` 只有新建、退出、CPU 增量超过阈值或 CPU 不可得才视为 active；检测失败不冒充 idle。
- driver 增加 `events.jsonl` append-only 即时记录。每个 command/source/idle_probe/sample/section_parse/fatal/finish 立即落盘；correctness、trace 或 parser 失败直接停止依赖测量，并写出失败 `raw.json`。
- 类型查询删除未使用的 `query/Input/Pack` 脚手架，同一源码内覆盖 found、not-found、empty 三个边界，运行结果依赖真实 trait 值。
- timing 后置检查改为即时 `active_build_probe()`，并比较计时前最后 snapshot 与计时后第一 snapshot；`wait_for_idle()` 只用于下一次采样前。
- full 模式新增硬门：前置 idle 失败即 FAIL；6 个 type_query 组和 2 个 multi 组都必须各有 1 个成功预热和 5 个有效样本，才允许 PASS 和发布 `latest-summary.md`。
- check-only 状态改为 `CHECK_ONLY_PASS`，只声明 correctness、trace 和 parser，不发布 `latest-summary.md`。
- 环境记录补充 `clang --version`、OS、CPU fallback、源码/对象/exe SHA-256；hash 读取在命令结束后执行，不计入 `process_seconds`。
- 环境记录补充 `time.get_clock_info("monotonic")` 和 `time.get_clock_info("perf_counter")`；计时协议仍保持 C01 runner 的 `time.monotonic()`，不更换计时算法。

## 验证

- `01-b01-configure.json`：`cmake -S C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost -B C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/build/local -G "Visual Studio 18 2026" -A x64` -> PASS。
- `02-b01-build.json`：`cmake --build C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/build/local --config Release` -> PASS。
- `03-b01-ctest.json`：`ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/build/local -C Release --output-on-failure` -> PASS。
- `06-b01-build-after-uint32.json`：修改 uint32_t 后重新 build -> PASS。
- `08-b01-ctest-after-uint32.json`：修改 uint32_t 后重新 CTest -> PASS，`1/1 Test #1: B01_compile_cost_contracts ... Passed`。
- `07-py-compile-driver.json`：`python -m py_compile C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py` -> PASS。
- `driver-checks/compile-cost-run-20260909-233349/`：保留失败即停证据，原因是旧 driver 未创建 obj 输出目录。
- `driver-checks/compile-cost-run-20260909-233545/`：`--check-only` PASS，覆盖 baseline compile/link/run、trace compile、section parser、symbol/size 命令和 implicit/explicit stdout equality。多 TU 输出均为 `3239304737 2665584707 2417941413 318504391`。
- `driver-checks/compile-cost-run-20260910-001031/`：新版 `--check-only` PASS，状态为 `CHECK_ONLY_PASS`，`latest_summary=null`。detector 控制覆盖驻留 MSBuild 放行、busy/new/exited/detection failure 拒绝，以及 4 个有效样本不能总体 PASS。implicit/explicit stdout 仍完全一致。
- `10-full-driver-after-warmup-gate.json`：第一次 full PASS，但随后发现生成摘要边界仍有旧的 name/PID 文案；结果目录保留，不作为最终正文链接。
- `11-full-driver-after-summary-boundary.json`：此前 full PASS，外层 recorder 耗时 191.109s，结果目录 `references/benchmarks/results/compile-cost-run-20260910-003243/`，历史结果保留。8 组样本门均为 1 个成功预热和 5 个有效样本。
- `12-full-driver-after-clock-boundary.json`：clock 边界修复后重新 full PASS，外层 recorder 耗时 194.031s，最终结果目录 `references/benchmarks/results/compile-cost-run-20260910-004959/`，`latest-summary.md` 已发布。8 组样本门均为 1 个成功预热和 5 个有效样本。

## 正式结果

- 类型查询 32 项：recursive 中位数 0.0940s，范围 0.0930-0.1100s；fold 中位数 0.0940s，范围 0.0940-0.1250s。
- 类型查询 128 项：recursive 中位数 0.1100s，范围 0.1090-0.1410s；fold 中位数 0.1090s，范围 0.0940-0.1250s。
- 类型查询 256 项：recursive 中位数 0.1570s，范围 0.1400-0.1720s；fold 中位数 0.1090s，范围 0.0930-0.1250s。
- 类型查询 trace `Total Frontend`：recursive 32/128/256 为 23173/37588/79812us；fold 为 22147/22193/25670us。`Total InstantiateClass` scope 计数为 recursive 6、fold 3，属于粗粒度定位线索，不是所有嵌套实例化总数，也不单独当 CPU 占比。
- 多 TU 编译：implicit 中位数 0.3750s，范围 0.3440-0.4220s；explicit 中位数 0.4380s，范围 0.4220-0.4530s。
- 多 TU section：调用方 `.obj` `.text` raw 合计 implicit 336、explicit 28；全部 `.obj` `.text` raw 合计 implicit 705、explicit 474；最终 `.exe` `.text` virtual 两者同为 90861。
- 多 TU stdout equality：implicit 与 explicit 均输出 `3239304737 2665584707 2417941413 318504391`。
- clock 边界：主线 `clock-resolution.json` 记录 `time.monotonic()` 为 `GetTickCount64()`，resolution 0.015625s；4 位小数只为展示和复算，不能从 1ms 级差异推断收益。32/128 项结论改为“本次未分辨出稳定收益”，不证明性能相等。`Total InstantiateClass count` 明确为 trace `Total ...` scope 计数，不是所有嵌套实例化总数。

## 采样阻断

- 第一次正式 driver 启动后检测到 `cmake`、`link`、`MSBuild` 和大量 `cl` 进程，已中断，不纳入结论。
- 第二次在外部进程短暂清空后重启 driver，随后再次检测到大量 `MSBuild` 进程并持续等待，已中断，不纳入结论。
- `04-process-snapshot-before-benchmark.json`：只读快照命令因 PowerShell 外层插值错误失败，保留为失败证据。
- `05-process-snapshot-before-benchmark.json`：修正转义后 PASS，stdout 显示 `cmake`、`link` 和多个 `MSBuild` PID。未输出完整 process command line。

## 剩余步骤

1. 交非作者 cost-review 复算 `references/benchmarks/results/compile-cost-run-20260910-004959/raw.json` 与 `events.jsonl`。

当前状态：formal sampling complete，构建窗口已开放。
