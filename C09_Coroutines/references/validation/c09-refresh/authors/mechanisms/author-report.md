# C09 D/E/F/G mechanisms author report

日期：2026-09-11。范围：D1/D3/G2 starter 有效性、F3 HALO 同契约实验与证据边界；D2/E1/E2/E3/F1/F2/G1 只做机制入口审计，未改代码。

## 修改结论

- D1 `main.cpp` 保留学生 TODO 起点，默认 `return_value` 不写结果；运行时真实创建 `compute()` 并调用学生 `lazy_task::get()`，未完成时 0.02 秒内失败：`TODO: return_value must store the co_return value`。
- D3 `main.cpp` 保留学生 TODO 起点，默认 final awaiter 不返回 continuation；运行时真实跑 `chain_sym(10).get()`，未完成时 0.02 秒内失败：`TODO: final_suspend must return continuation when present`。
- G2 `main.cpp` 增加 gated child 检测。顺序 drain starter 会在启动第二个 child 前跑完第一个 child，0.46 秒内失败：`TODO: when_all_2 must start both child tasks before draining either one`。
- 三题各自 `CMakeLists.txt` 使用公共 `STUDENT_TEST` 参数，在 `COROUTINE_STUDY_TEST_STARTERS=ON` 时注册 starter CTest；G2 保留 5 秒 timeout。
- F3 starter 改为同契约 A/B：两版都计算 `0 + ... + (n - 1)`；使用 `steady_clock`、计时内无日志、轻量求和校验。新增 `sample_f3.py`：1 次 warmup，5 次独立进程，固定 seed 打乱版本顺序，输出全部样本、median、min/max。
- F3 默认保留 allocation hook 作为插桩观察；新增 `F3_UNINSTRUMENTED_HALO`，同一源码可关闭 hook 生成无插桩 IR/asm。

## 机制审计

| 单元 | 类型 | 结论 |
|---|---|---|
| D1 | 实现型 | 已修成真实 student path；默认 TODO 有限失败；Reference 独立通过。 |
| D2 | 观察型 | 入口完整可运行，观察 lazy/eager 启动时序；通过只证明日志路径，不证明学生实现。 |
| D3 | 实现型 | 已修成真实 student path；默认 TODO 有限失败；Reference 独立通过。 |
| E1 | 观察型 | 展示 `await_transform`、成员 `operator co_await`、ADL 查找；通过只证明观察输出。 |
| E2 | 观察型 | 展示 `void/bool/handle` 三类 `await_suspend`；通过只证明控制流观察。 |
| E3 | 观察型 | 展示 ready 短路；优化/HALO 结论不能由本题耗时推出。 |
| F1 | 观察型 | frame layout dump 入口；正文已区分标准语义与实现布局。 |
| F2 | 观察型 | allocator hook 入口；正文已区分 throwing allocation 与 allocation-failure hook。 |
| F3 | 实验型 | 已去掉倍率门槛；当前 Windows 采样只能作线索，Clang 无插桩 IR/asm 未显示 elision。 |
| G1 | 观察/演示型 | shared_task 多等待者入口；扩大到线程安全需另行实现，不在本切片改。 |
| G2 | 实现型 | 已修成真实 fan-out 检测；顺序 drain 不能假通过；Reference 独立通过。 |

## F3 证据

改前 baseline：`baseline/03-run-f3-starter.output.txt` 显示旧程序仍输出固定 ratio 叙事，本机为 `ratio B/A 1.02x`；`baseline/04-run-f3-reference.output.txt` 是另一个 hook fixture，和 starter benchmark 不是同一契约。

改后 correctness：`23-run-f3-starter-final.output.txt` 显示两版求和检查通过，插桩 allocation/deallocation 为 `2/2`。这是插桩计数，不作为无插桩 HALO 证明。

改后采样：`24-f3-samples-final.output.txt`：

```text
summary,version=local,median=115.037,min=108.407,max=121.706,samples=118.218;108.407;115.037;121.706;112.345
summary,version=escaped,median=125.183,min=116.156,max=150.264,samples=150.264;116.156;120.058;125.183;140.282
```

无插桩诊断：`19-wsl-clang-version.output.txt` 为 Ubuntu Clang 18.1.3。`20-f3-uninstrumented-ir.command.txt` 与 `21-f3-uninstrumented-asm.command.txt` 使用 `-DF3_UNINSTRUMENTED_HALO`。当前 `f3-uninstrumented.ll` 仍在 `range_values` 创建处包含 `_Znwm(i64 48)`，`f3-uninstrumented.s` 也有 `_Znwm@PLT` 调用；`22-f3-rpass.output.txt` 为空，未观察到 `coroutine-elide` remark。因此本轮只记录“当前 Clang 18/O2 未证明 HALO”，不声明触发。

## 命令索引

- `01-configure.command.txt`：Windows VS18 Release reference 配置。
- `12-build-after-fix.command.txt`：D1/D3/G2/F3 starter + reference 构建，exit 0。
- `13-ctest-reference-after-fix.command.txt`：D1/D3/G2/F3 reference，4/4 passed。
- `25-configure-student-helper.command.txt`：使用公共 `STUDENT_TEST` helper 刷新 student 配置，exit 0。
- `28-build-student-helper-seq.command.txt`：D1/D3/G2 starter 顺序构建，exit 0。
- `29-ctest-student-helper-seq.command.txt`：D1/D3/G2 starter，3/3 按预期失败，exit 8。
- `23-run-f3-starter-final.command.txt`：F3 correctness 与插桩计数，exit 0。
- `24-f3-samples-final.command.txt`：F3 1+5 独立进程采样，exit 0。
- `20-f3-uninstrumented-ir.command.txt`、`21-f3-uninstrumented-asm.command.txt`、`22-f3-rpass.command.txt`：WSL Clang 无插桩诊断，exit 0。

## 待非作者复验点

- 检查 D1/D3/G2 默认 starter 失败是否来自学生 TODO，而不是检查器常量或找不到可执行文件。
- 遮住 Reference，沿 D1/D3/G2 README 判断 TODO 是否可完成。
- 复核 F3 文档是否明确：timing 是线索，allocation hook 是插桩观察，无插桩 IR/asm/remark 才能支撑 HALO 结论。
- 公共 `AddExercise.cmake` 的 `STUDENT_TEST` 已由公共负责人提供；本切片只在题内使用该参数。
