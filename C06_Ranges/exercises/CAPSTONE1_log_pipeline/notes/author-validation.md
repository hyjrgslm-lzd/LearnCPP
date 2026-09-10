# CAPSTONE1 作者验证记录

范围：`C06_Ranges/exercises/CAPSTONE1_log_pipeline/**` 和 `C06_Ranges/chapters/00-log-pipeline-evolution.md`。

配置阶段观察到的环境：

- 生成器：Visual Studio 18 2026
- 编译器：MSVC 19.51.36256.0
- 构建目录：`C06_Ranges/exercises/CAPSTONE1_log_pipeline/build-sample-author`
- 配置：Release

## 命令与结果

```powershell
cmake -S C06_Ranges\exercises\CAPSTONE1_log_pipeline -B C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author
```

结果：exit 0。配置和生成完成。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_reference
```

第一次结果：exit 1。MSVC 拒绝用 `take_view` 的 iterator/sentinel pair 构造 `std::vector<LogRecord>`，因为 sentinel 类型不同。后续修复曾改成显式循环收束；r2 已按 review 要求恢复为 `std::ranges::to`，并重新验证通过。

最终结果：exit 0。`CAPSTONE1_log_pipeline_reference.exe` 构建完成。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_validation_good
```

结果：exit 0。`CAPSTONE1_log_pipeline_validation_good.exe` 构建完成。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_validation_bad
```

第一次并行结果：exit 1。VS `ZERO_CHECK.tlog` 被另一个 MSBuild 进程占用；改为串行重试。

最终结果：exit 0。`CAPSTONE1_log_pipeline_validation_bad.exe` 构建完成。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_student
```

结果：exit 0。`CAPSTONE1_log_pipeline_student.exe` 构建完成。

```powershell
ctest --test-dir C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author -C Release --output-on-failure
```

结果：exit 0。3/3 通过：

- `CAPSTONE1_log_pipeline_reference`
- `CAPSTONE1_log_pipeline_validation_good`
- `CAPSTONE1_log_pipeline_validation_bad_rejected`

```powershell
C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author\Release\CAPSTONE1_log_pipeline_student.exe
```

结果：exit 1，输出：

```text
check failed: valid row parses
```

这证明未完成 student 实现安全、有限，但会被行为检查拒绝。

## 实测操作计数

checker 对固定样例输入验证：

- 输入行数：9
- 有效记录：7
- 循环基线解析次数：9
- 修正后的 ranges 解析次数：9
- 直接安全 `transform(parse)->filter(optional)->transform(unwrap)` 重复解析 demo 次数：16
- bad 预期拒绝文本：`valid records must parse each input line once`

没有做计时或加速结论。
